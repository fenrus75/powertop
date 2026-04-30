#!/usr/bin/env python3
import sys
import base64
import argparse
import os
import tempfile
import subprocess
import re
import shutil

def load_trace(trace_file):
    try:
        with open(trace_file, 'r') as f:
            return f.readlines()
    except Exception as e:
        print(f"Error opening trace file: {e}")
        sys.exit(1)

def save_trace(trace_file, lines):
    try:
        with open(trace_file, 'w') as f:
            f.writelines(lines)
    except Exception as e:
        print(f"Error writing to trace file: {e}")
        sys.exit(1)

def parse_line(line, line_num):
    line = line.strip()
    if not line: return None
    first_space = line.find(' ')
    if first_space == -1: return None
    tag = line[:first_space]
    rest = line[first_space+1:]
    if tag == 'N':
        return tag, rest, None
    if tag == 'M':
        return tag, rest, None
    if tag == 'T':
        return tag, rest, None
    if tag == 'L':
        # Format: L b64(target) path
        # b64 comes FIRST (may be empty = failed readlink: "L  path")
        # path comes SECOND and may contain spaces
        sep = rest.find(' ')
        if sep == -1:
            return tag, '', rest   # degenerate: no space
        b64 = rest[:sep]
        path = rest[sep+1:]
        return tag, path, b64
    last_space = rest.rfind(' ')
    if last_space == -1: return None
    path = rest[:last_space]
    b64 = rest[last_space+1:]
    return tag, path, b64

def get_tag_str(tag):
    if tag == 'R': return "Read"
    if tag == 'W': return "Write"
    if tag == 'N': return "Miss"
    if tag == 'M': return "MSR"
    if tag == 'T': return "Time"
    if tag == 'L': return "Link"
    return "????"

def decode_content(tag, b64):
    """Return human-readable content for display, or None if not applicable."""
    if tag in ('N', 'M', 'T') or b64 is None:
        return None
    if tag == 'L':
        if not b64:
            return "(broken link)"
        try:
            return base64.b64decode(b64).decode('utf-8', errors='replace').strip()
        except Exception:
            return None
    try:
        return base64.b64decode(b64).decode('utf-8', errors='replace').strip()
    except Exception:
        return None

def cmd_list(args):
    lines = load_trace(args.trace_file)
    show_content = getattr(args, 'content', False)
    path_filter = getattr(args, 'path', None)
    if show_content:
        print(f"{'Line':<6} {'Type':<6} {'Path':<55} {'Content'}")
        print("-" * 100)
    else:
        print(f"{'Line':<6} {'Type':<6} {'Path'}")
        print("-" * 60)
    for i, line in enumerate(lines, 1):
        parsed = parse_line(line, i)
        if not parsed:
            continue
        tag, path, b64 = parsed
        if path_filter and path_filter not in path:
            continue
        if show_content:
            content = decode_content(tag, b64) or ""
            # Truncate long content for display
            if len(content) > 40:
                content = content[:37] + "..."
            print(f"{i:<6} {get_tag_str(tag):<6} {path:<55} {content}")
        else:
            print(f"{i:<6} {get_tag_str(tag):<6} {path}")

def cmd_extract(args):
    lines = load_trace(args.trace_file)
    if args.line < 1 or args.line > len(lines):
        print(f"Error: Line number {args.line} out of range (1-{len(lines)})")
        sys.exit(1)

    parsed = parse_line(lines[args.line - 1], args.line)
    if not parsed:
        print(f"Error: Invalid line format at line {args.line}")
        sys.exit(1)

    tag, path, b64_content = parsed
    if tag == 'N':
        print(f"Error: Line {args.line} is a 'File Not Found' (Miss) entry. No content to extract.")
        sys.exit(1)
    if tag == 'L':
        if not b64_content:
            print(f"Line {args.line} is a broken symlink (empty target). Writing empty file.")
            with open(args.output_file, 'wb') as f:
                pass
            return
    if tag in ('M', 'T'):
        print(f"Error: Line {args.line} is a {get_tag_str(tag)} entry; extract writes decoded bytes.")

    try:
        content = base64.b64decode(b64_content)
        with open(args.output_file, 'wb') as f:
            f.write(content)
        print(f"Extracted line {args.line} to {args.output_file}")
    except Exception as e:
        print(f"Error: {e}")
        sys.exit(1)

def cmd_replace(args):
    lines = load_trace(args.trace_file)
    if args.line < 1 or args.line > len(lines):
        print(f"Error: Line number {args.line} out of range (1-{len(lines)})")
        sys.exit(1)

    try:
        with open(args.input_file, 'rb') as f:
            content = f.read()
    except Exception as e:
        print(f"Error opening input file: {e}")
        sys.exit(1)

    b64_content = base64.b64encode(content).decode('ascii')
    parsed = parse_line(lines[args.line - 1], args.line)
    if not parsed:
        print(f"Error: Invalid line format at line {args.line}")
        sys.exit(1)

    tag, path, _ = parsed
    if tag == 'N':
        # Miss → Read with content
        lines[args.line - 1] = f"R {path} {b64_content}\n"
    elif tag == 'L':
        # Replace symlink target; preserve path
        lines[args.line - 1] = f"L {b64_content} {path}\n"
    else:
        lines[args.line - 1] = f"{tag} {path} {b64_content}\n"
    save_trace(args.trace_file, lines)
    print(f"Replaced content at line {args.line} with {args.input_file}")

def cmd_edit(args):
    lines = load_trace(args.trace_file)
    if args.line < 1 or args.line > len(lines):
        print(f"Error: Line number {args.line} out of range (1-{len(lines)})")
        sys.exit(1)

    parsed = parse_line(lines[args.line - 1], args.line)
    if not parsed:
        print(f"Error: Invalid line format at line {args.line}")
        sys.exit(1)

    tag, path, b64_content = parsed
    if tag == 'N':
        print(f"Error: Line {args.line} is a 'File Not Found' (Miss) entry. No content to edit.")
        sys.exit(1)
    if tag in ('M', 'T'):
        print(f"Error: Line {args.line} is a {get_tag_str(tag)} entry; use direct file editing.")
        sys.exit(1)

    try:
        content = base64.b64decode(b64_content) if b64_content else b''
    except Exception as e:
        print(f"Error decoding base64: {e}")
        sys.exit(1)

    fd, temp_path = tempfile.mkstemp()
    try:
        with os.fdopen(fd, 'wb') as tmp:
            tmp.write(content)
        
        editor = os.environ.get('EDITOR', 'joe')
        subprocess.run([editor, temp_path], check=True)
        
        with open(temp_path, 'rb') as f:
            new_content = f.read()
        
        new_b64 = base64.b64encode(new_content).decode('ascii')
        if tag == 'L':
            lines[args.line - 1] = f"L {new_b64} {path}\n"
        else:
            lines[args.line - 1] = f"{tag} {path} {new_b64}\n"
        save_trace(args.trace_file, lines)
        print(f"Successfully edited line {args.line}")
        
    except Exception as e:
        print(f"Error during edit: {e}")
        sys.exit(1)
    finally:
        if os.path.exists(temp_path):
            os.remove(temp_path)

def cmd_search(args):
    lines = load_trace(args.trace_file)
    pattern = re.compile(args.pattern)
    print(f"{'Line':<6} {'Type':<6} {'Path'}")
    print("-" * 60)
    for i, line in enumerate(lines, 1):
        parsed = parse_line(line, i)
        if not parsed: continue
        tag, path, _ = parsed
        if pattern.search(path):
            print(f"{i:<6} {get_tag_str(tag):<6} {path}")

def cmd_grep(args):
    lines = load_trace(args.trace_file)
    search_bytes = args.string.encode('utf-8')
    print(f"{'Line':<6} {'Type':<6} {'Path'}")
    print("-" * 60)
    for i, line in enumerate(lines, 1):
        parsed = parse_line(line, i)
        if not parsed: continue
        tag, path, b64 = parsed
        if tag == 'N': continue
        try:
            content = base64.b64decode(b64)
            if search_bytes in content:
                print(f"{i:<6} {get_tag_str(tag):<6} {path}")
        except:
            continue

def cmd_export(args):
    lines = load_trace(args.trace_file)
    if not os.path.exists(args.output_dir):
        os.makedirs(args.output_dir)
    
    for i, line in enumerate(lines, 1):
        parsed = parse_line(line, i)
        if not parsed: continue
        tag, path, b64 = parsed
        if tag == 'N': continue
        
        # Sanitize path for filename
        safe_path = path.replace('/', '_').replace(' ', '_').strip('_')
        filename = f"{i:04d}_{tag}_{safe_path}"
        dest = os.path.join(args.output_dir, filename)
        
        try:
            content = base64.b64decode(b64)
            with open(dest, 'wb') as f:
                f.write(content)
        except Exception as e:
            print(f"Warning: Failed to export line {i}: {e}")
    
    print(f"Exported entries to {args.output_dir}")

def cmd_validate(args):
    lines = load_trace(args.trace_file)
    errors = 0
    for i, line in enumerate(lines, 1):
        if not line.strip(): continue
        parsed = parse_line(line, i)
        if not parsed:
            print(f"Line {i}: Invalid format")
            errors += 1
            continue
        tag, path, b64 = parsed
        if tag not in ['R', 'W', 'N', 'M', 'T', 'L']:
            print(f"Line {i}: Invalid tag '{tag}'")
            errors += 1
        if tag == 'M':
            parts = path.split(' ')
            if len(parts) != 3:
                print(f"Line {i}: Invalid MSR format (expected cpu offset value)")
                errors += 1
            else:
                try:
                    int(parts[1], 16)
                    int(parts[2], 16)
                except:
                    print(f"Line {i}: Invalid MSR hex value")
                    errors += 1
        if tag == 'T':
            parts = path.split(' ')
            if len(parts) != 2:
                print(f"Line {i}: Invalid Time format (expected sec usec)")
                errors += 1
            else:
                try:
                    int(parts[0])
                    int(parts[1])
                except:
                    print(f"Line {i}: Invalid Time decimal value")
                    errors += 1
        if tag == 'L':
            # b64 may be empty (broken link); if non-empty, must be valid base64
            if b64:
                try:
                    base64.b64decode(b64)
                except:
                    print(f"Line {i}: Invalid base64 in link target")
                    errors += 1
            if not path:
                print(f"Line {i}: Link record missing path")
                errors += 1
        if tag in ['R', 'W'] and b64:
            try:
                base64.b64decode(b64)
            except:
                print(f"Line {i}: Invalid base64 content")
                errors += 1
    
    if errors == 0:
        print("Trace file is valid.")
    else:
        print(f"Validation failed with {errors} errors.")
        sys.exit(1)

def cmd_add(args):
    """Append a new record to a trace file, creating it if necessary."""
    record_type = args.record_type.upper()
    path = args.path
    value = args.value or ""

    if record_type == 'R':
        b64 = base64.b64encode(value.encode('utf-8')).decode('ascii')
        record = f"R {path} {b64}\n"
    elif record_type == 'W':
        b64 = base64.b64encode(value.encode('utf-8')).decode('ascii')
        record = f"W {path} {b64}\n"
    elif record_type == 'N':
        record = f"N {path}\n"
    elif record_type == 'L':
        # path = symlink path, value = target (empty = broken link)
        b64 = base64.b64encode(value.encode('utf-8')).decode('ascii') if value else ""
        record = f"L {b64} {path}\n"
    elif record_type == 'T':
        # path = "sec usec"  (two decimal integers)
        parts = path.split()
        if len(parts) != 2:
            print("Error: T record requires path to be 'sec usec' (two integers).")
            sys.exit(1)
        try:
            int(parts[0]); int(parts[1])
        except ValueError:
            print("Error: T record sec and usec must be integers.")
            sys.exit(1)
        record = f"T {parts[0]} {parts[1]}\n"
    else:
        print(f"Error: Unknown record type '{record_type}'. Use R, W, N, L, or T.")
        sys.exit(1)

    try:
        with open(args.trace_file, 'a') as f:
            f.write(record)
        print(f"Added: {record.strip()}")
    except Exception as e:
        print(f"Error writing to trace file: {e}")
        sys.exit(1)

def main():
    parser = argparse.ArgumentParser(description="PowerTOP Trace Tool")
    subparsers = parser.add_subparsers(dest="command", required=True)

    # list
    p = subparsers.add_parser("list", help="List all entries")
    p.add_argument("trace_file")
    p.add_argument("--content", "-c", action="store_true",
                   help="Show decoded content inline")
    p.add_argument("--path", "-p", metavar="SUBSTR",
                   help="Only show entries whose path contains SUBSTR")

    # extract
    p = subparsers.add_parser("extract", help="Extract a line")
    p.add_argument("trace_file")
    p.add_argument("line", type=int)
    p.add_argument("output_file")
    
    # replace
    p = subparsers.add_parser("replace", help="Replace a line")
    p.add_argument("trace_file")
    p.add_argument("line", type=int)
    p.add_argument("input_file")
    
    # edit
    p = subparsers.add_parser("edit", help="Edit a line in-place")
    p.add_argument("trace_file")
    p.add_argument("line", type=int)

    # search
    p = subparsers.add_parser("search", help="Search paths by regex")
    p.add_argument("trace_file")
    p.add_argument("pattern")

    # grep
    p = subparsers.add_parser("grep", help="Search content for string")
    p.add_argument("trace_file")
    p.add_argument("string")

    # export
    p = subparsers.add_parser("export", help="Export all entries to a directory")
    p.add_argument("trace_file")
    p.add_argument("output_dir")

    # validate
    subparsers.add_parser("validate", help="Validate trace file format").add_argument("trace_file")

    # add
    p = subparsers.add_parser("add",
        help="Append a record to a trace file (creates file if needed)")
    p.add_argument("trace_file")
    p.add_argument("record_type", metavar="type", choices=["R", "W", "N", "L", "T"],
                   help="Record type: R=read, W=write, N=miss, L=symlink")
    p.add_argument("path", help="Sysfs/proc path (for L: the symlink path)")
    p.add_argument("value", nargs="?", default="",
                   help="Content string (for L: symlink target; omit for broken link or N)")

    args = parser.parse_args()
    
    cmds = {
        "list": cmd_list, "extract": cmd_extract, "replace": cmd_replace,
        "edit": cmd_edit, "search": cmd_search, "grep": cmd_grep,
        "export": cmd_export, "validate": cmd_validate, "add": cmd_add,
    }
    cmds[args.command](args)

if __name__ == "__main__":
    main()
