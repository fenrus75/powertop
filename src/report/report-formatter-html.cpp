/* Copyright (c) 2012 Samsung Electronics Co., Ltd.
 *	http://www.samsung.com/
 *
 * This file is part of PowerTOP
 *
 * This program file is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program in a file named COPYING; if not, write to the
 * Free Software Foundation, Inc,
 * 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA
 * or just google for it.
 *
 * HTML report generator.
 * Written by Igor Zhbanov <i.zhbanov@samsung.com>
 * 2012.10 */

#define _BSD_SOURCE

/* Uncomment to disable asserts */
/*#define NDEBUG*/

#include <stdio.h>
#include <assert.h>
#include <stdarg.h>

#include "report-formatter-html.h"
#include "css.h" /* For HTML-report header */

/* ************************************************************************ */

#ifdef EXTERNAL_CSS_FILE /* Where is it defined? */
static const char report_html_alternative_head[] =
	"<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\" "
		"\"http://www.w3.org/TR/html4/loose.dtd\">\n"
	"<html>\n"
	"<head>\n"
	"<title>PowerTOP report</title>\n"
	"<link rel=\"stylesheet\" href=\"powertop.css\">\n"
	"</head>\n"
	"<body>\n";
#endif /* EXTERNAL_CSS_FILE */

/* ************************************************************************ */

static const char report_html_footer[] =
	"</div>\n"
	"</body>\n"
	"</html>\n";

/* ************************************************************************ */

void
report_formatter_html::init_markup()
{
	/*here all html code*/
}

/* ************************************************************************ */

report_formatter_html::report_formatter_html()
{
	init_markup();
	add_doc_header();
}

/* ************************************************************************ */

void
report_formatter_html::finish_report()
{
	add_doc_footer();
}

/* ************************************************************************ */

void
report_formatter_html::add_doc_header()
{
#ifdef EXTERNAL_CSS_FILE /* Where is it defined? */
	add_exact(report_html_alternative_head);
#else /* !EXTERNAL_CSS_FILE */
	add_exact(css);
#endif /* !EXTERNAL_CSS_FILE */
}

/* ************************************************************************ */

void
report_formatter_html::add_doc_footer()
{
	add_exact(report_html_footer);
}

/* ************************************************************************ */
string
report_formatter_html::escape_string(const char *str)
{
	string res;

	assert(str);

	for (const char *i = str; *i; i++) {
		switch (*i) {
			case '<':
				res += "&lt;";
				continue;
			case '>':
				res += "&gt;";
				continue;
			case '&':
				res += "&amp;";
				continue;
#ifdef REPORT_HTML_ESCAPE_QUOTES
			case '"':
				res += "&quot;";
				continue;
			case '\'':
				res += "&apos;";
				continue;
#endif /* REPORT_HTML_ESCAPE_QUOTES */
		}

		res += *i;
	}

	return res;
}


/* Report Style */
void
report_formatter_html::add_logo()
{
	add_exact("<img alt=\"PowerTop\" class=\"pwtop_logo\" src=\"data:image/png;base64,"
			"iVBORw0KGgoAAAANSUhEUgAAAbQAAABDCAYAAAD01PBTAAAACXBIWXMAAAsTAAALEwEAmpwYAAAAG"
			"XRFWHRTb2Z0d2FyZQBBZG9iZSBJbWFnZVJlYWR5ccllPAAAGsxJREFUeNrsXQt4FNXZ/nLhmutqV"
			"dAKMXJHTMhiLGAuLUEfSg2xSFSUgIkkRPQ3QiHxqcRwswQrRq2EBKjhapvU3yCgtaQ+CRGUSyABv"
			"NDfRhArqEA2FwjksvnPN0xokpnZPWd2dnd297w851kyt3OZ+c4773e+c8aro6MDODg4ODg4XB3ev"
			"Ak4ODg4ODihcXBwcHBwcELj4ODg4ODQDr68CWxCrPgbTlJwj33l4m81SSbeVBwcHBz2hRcPCqEGE"
			"laCSGKYBldXV4PJZILO325MF3uN68LDwyE4OLheJLjOVM2bk4ODg4MTmqMxB4mMENa08vJyKC0tF"
			"QispqaG+gKDBw8WiA1JLiEhAUJCQk6TzXkklZJ0ijcxBwcHByc0e6qxDEyExILy8vJgx44dml08L"
			"CwMMjIyBHIj6m0T2ZTDiY2Dwz3xzsEbM3tsWkV5ahVJJV3/fizyQhlvURsJzcvLaw/5iXNQmcrEd"
			"P1vUsYqRzUIyQuJLKeoqCho6dKlcPr0abvlFRQUJBDbc88910lsGaStVY23kfMyGQwFcQOpa52Kf"
			"IrJzwwVRVSbXwH5SdU6Hwc/0ywoIeVP1FHZe3aqNtsj6eDVPkMOvxeEQBJV1A+fVyPjc8uCWpIK8"
			"V6Q8mnaN5KyO7Wvt7U+egwKievZoMSAQTQqNPYSe2Ta3t6OgR1FFRUVYcnJyXYlsk7U19cDkiYqw"
			"Ozs7NmE2HCMbo6Pj0+pyoeDBaFiZwUqzlMDo4oysuZXpYY0dQa9ld8oJi3t0eAi96JWhRJb5YByh"
			"XbmQ/JEG84lRFDigs+6pK8n9bn+bKmpkyuF7eMbXTExpsMkGTUls7b2DFOd6eiC5xeETZo0iZrMv"
			"G+6EXxHD4e+UydD34d+DYFD7oDFixZD9pJsiI+Ph+joaGpiW7hwIURERATVVNe8R8pTSlIwo7KsY"
			"uwMbSEmR57HUk53cMfUuqA9xtnxnur+5YJ0wjNIuuggMpOzq2JUViSFgntghlinwyQx9RuuGLaPF"
			"UQjSiOdeKGtF2trayuqqamZPX36dKtE5tW/H/SOjIBe946FXqNHgJdf/277J9x4C6yceL/kPKL6h"
			"ITjcMeOHVO8Pu4zjjPCxo0bpyUlJZWTss3x9fVliYisZSAO5offxhcJNfkZWBWaGxhznQva4x5Ge"
			"zS4y73QkfsUXyr+TcqT6KJqTbGvJ3VKI3WierZceWJ1ATEi1Q9Sa2trMEnVmzZtmm1NlaEK83smB"
			"Qxb3gK/Z1MEUutJZohdsVNRKUkSKrUlS5bA4cOH4dChQzBr1iyLZUtJSYHk5OQwcm45KWO4nRSKG"
			"nIKtfHhtHd+tW5gxHUeYI8u73IknawBFQTobyywWCYIxdVRgCrY3QkNsUp8i2fC1atXg81mczkhs"
			"7CnnnpKcPnJwSfkdghYthgCl2VCn19OpLo2ua7FNGbMGFi/fj2cPHkSnnjiCcXrbNmyBUkt6MKFC"
			"xWkvLSkxqJQ1JCTLQrNqOJesZSxzpHBQ5zQFEnNYEXRhLrJvdhjoz3YtV90Q1LDOlntP1yd0NA4m"
			"COJUPls3rw5bO7cubL70bXY/8nHIOjVpYJrkRY/f2CSrEKTS4MGDRKI7aOPPhL+L4etW7dCampqI"
			"Jb3ypUrNKRWxth2jlRoas73tPEzV1eZBgp7dHlCIx3rKh2TWVcCMIL7gKqvd4elr7CSubQHNzc3F"
			"xH1E5aWlqaoyvyfSQGfOwYxF+TurOeBdV5fVFQUHDhwABYtWiQQWE/s3LkTCPEGFRYWlpKyh/fr1"
			"89kgajryBtyFaWxGcixoeQclg7UVgOJY1SRLIquyk0M19WjNDOt2KOruBvhscgLtTJkNkOsIyuuh"
			"6iT61L1VySvuC42pybgBN2P40h+dW5iG1b7endYnDiUNlih+fLljGM1NbORPGTZffRwwb2ohswQx"
			"1a9hqzCnIICA6GwoABW5+YqKrWVK1YMJseWUhqO5gSlIkDD0QrNHcbPwA2mHRisRD26ekAIK7GgP"
			"SKpTEYioyUzkfjKOs8hCedKZNlD1bhSX29NdWqt0GqJQd6p9mRiCGonJFp987906VJ4fX39a4mPP"
			"AINDQ3ShhgxVCAztWg5eAQufvRP2Lxli8WxMUt4ev58CAwKgnnz5kn2rXz5ZbgvKiqGKLocPz+/H"
			"I069lA7HauVwjMydhz2gE3PtJPhLHu0NOewhDZiTUG10E78rSX5aHrfxHEpFjtAIsrSKn8kNlIGb"
			"NdihnJkknMK7aDSbGpfGyafW+zrdaXQMOyXJPQFTtb6TZ5ctyg1NRW+/fZbyb7ekWMhcOULcHF6M"
			"lx6c6MqMmvK/ZPwfySjlStXUo+l9UyPP/44rFu3TjYfdJOaTKaXmpqaLI2n2SswRAt/vFHr+9pZZ"
			"zdQNnpUi532OE6r++zi7i+WzrdESzLr0n5o34lA75o2gA5VGr7UkKR5X++tU0MqY5TXFivZ2NiYs"
			"Xfv3rDdu3dL9uGYGYbkC41x041wtXwfM6m1f3Om29+VlZWqCQ3TzJkzIT09XZIPkrFIlnkW2o5lg"
			"rWjFRr1XDYc39OBOuP47zOV5ehnRU8QFQVtvdBLkmZHMhBWB2E4JZMmQtBJxKZpX6/nMTRNIvYaG"
			"hqCiUHmyLnxMJoRA0A655T1eyRB+EVS06ATsCmtWrUK7rrrLsl18/PzcQJ2DKlXggZtx6KYtIqYi"
			"tPiwe0Brs7sj0KtOh0XBctqKLn2VqLiWBzt8IIB9LluqaZ9va4JjVFpKL59mM3mjK1btwadOXNGs"
			"g8JrGsACM41w3lnmNS6HzvJzNp8NJq0bds2CAwMlL5uZWbi/jwLRaB1OxoY5obREFqhLffKhk6RK"
			"zT722MdYwfqTuoM60M7gRrbyVErdbCoNN0uBi0qzjotni29RznaVElTXV0wscQMVDs9IazB+Bup+"
			"xbnnWFS637s0gPYnAbdfrus63Hfvn1w/NixwaR+sRp08FaJSiQ9a51ULWW+Wis0d5lQ7QqgDjhid"
			"Bm7kzordOA4YQlDHxmn8zb2CEKzKRSbcELCrt27FdTZNIvn+o4aDv0DfFW7H63x1enT38LsJ5+Ah"
			"YueB5OpXvG4zMWZcDshtp7Iz1+H+zM0ULc0HQ+NOquivF+hGpaLRY1ycKgFi7vdYc+jSJy0L68Gn"
			"U+01mTajbc7P4XmDnPG9u3bZdWZpRVAmtIXQEvFfrjc2Cb83Tx7LpNSE1yOHWbF9Ld3/wbhY8Ph/"
			"R0fwJ83boKhw4fA8RPHFY9fvHix1A3yl3egzlQ37cKFCyE2PiCaERolkRooA0M4oXG4IqE52v1dZ"
			"ad6uCTcltDOnz8fYjKZwj788EPJPjlXYyda8wuh5cfui3FcudTO7H60FPDxcu6ybse2tbbDbx+OV"
			"zx+ypQpsmNpH3zwAe5PsNGwQjU6poohX05oHO5IaFVOmJZg7/VbOaFpCNWDy9jRY4cvqfBNNwqr5"
			"cuec+kyNH18QOZa135Z3I+WCO3KlSuS4y/8VA9r89+QPR7JDEmtJ3AaAtkfa+ODbtTomCqGfC0aF"
			"uWYnbPeiDk8CGJACO2z6IzVatyF0DQJJNI7oVEHBsgQSiwGT/SEEpkhWt4qgA6zdC3GXgZ/5uhHS"
			"4Q2zig/T/WVV/6oeM6jjz4qOX7//v24b5oOFFrXic00BmbU6L7X8gnVurRHWZv0gDo7nNAYFWGoG"
			"7RznUsSmjjOYlBbyQ6zOVyO0PDjnEq4cvQL2e39lyy6Hv3YqdQskZpAQmazYtpQ8Db06dtbcl5TY"
			"zMc+Owz2XPG/+IXErcjLuF1/Phx+OHcuViZMtTRvr1ZGtMSo9UMDIasBaFRu3gcYWikDTo0Tv92t"
			"V6ddS1PN3rRYFEOzqqzS0+nEINVDFq0sZ4VWpzaG3ru7Lng+vqGwd99952U0BSCQVqPHIP2lnYpm"
			"YUOBJ/B/40yxFVFOkP6LRu05ZS5SH5y/Btr1yieM2H8BMnxJ46fwH3hdlRpLO5GWiK1FhhC+3Dz8"
			"TPHIVWtPXJVqhtC06tCi9OqrrokNHG17lVqK0n0UThGDfYELnOlSEAywSMCAT7TfYURnHwdvO4Vq"
			"+7HDiv/5qXPIyqtl+S8z/YfVDxn9F2jJcef+e4M7gu244NOYwRlKog0VAPD4+NnLmCPHBwW1Jmmz"
			"5avzgxH7QrMtT3cHcFyX6H2uflnihdo/Uaq5nz6+HZTZ3JKD5WaHFHSfBfNeM9Y2F95sNs2dDsqn"
			"Ttq1CipQjtxQovAEFsVWq2KfPG6JbYQGp9QrVt79NT7wsdz6YlMk77e3oQmjDc4oX26vambzeZw7"
			"OilCk35O2ftzS2SbX1uu8lipuh+bP5rKbSfOiMhM1y6yhrGR0ZJCA3x+eefw8iRIyXbAwICJNuQu"
			"JXywg6f3A80MmsuPKMNaklupXtbIx1Zpglw6M8e3aljd6cxNCSSULkPl6p5tsi1nN7X94Q7zEOTL"
			"n3Uwd7ObVdapY1z60CL53S6H/slTpNjE6vp3shI2esKwSwyx1++dElJqqh+ADRQaFUyRFpLYdxxC"
			"qqANviAuxv1ixLeBBz26OvFdR/dmtByZTpU2U6+7V/Xgstw7Kvr+JfSOFjTJ0fAGGCAvuu2XD8H/"
			"4/buv49sE/fbuc1trVC0rPz4fZBg4R0qOFit7/x/7gNPzYqh+J3C8F09RA8k5F0/Rz8+72dm2S4r"
			"M2ae5NGyRjk1t6jXNGjViXhGBTW++MTql0bZeILDQeH3fv6nvB18QqiCiiUIzS5Lv6p8VGQZfwVw"
			"KlT3XcU/QoGh4RIjn/44Yfh1WFjAda/Lb1Yl2u8lpcHXZe+P9ncBGeTHoIbSBLyPXkEoMvfNeI2D"
			"CxpzF4tufT0+U1w/OJceCQTSLpB2IZ/j/9tO+z43x6kAz7WCI3lUzK1KsilzALhzLBTnpzQXLjTc"
			"cE+hhbOCounjl7UyN2om77e3RRamtx8F7O5Q7aT7927t7BPLvn28pEc/58fvlE8vmvq6NDWleznL"
			"39bmhqlY2UdYn2VwLBQcagC4ViEhcAMtUtg0RhnHVcBukSJ+HFeT4WBPwL26+tpJpG7skJDMpP11"
			"ROKMckFUOBEZHntBsJE57bW5m7bTn75teLxWqDtxEnZ7YOH+TBdh6KMtRTkZFBBLlVW8lRDojSdg"
			"iM7TVyN5E7en1Ap5jTeDBx2IjOqcVlvFzWcyaSTKbSgGqrlQty/+OILxaWlQkJ/Ljn+4vl6IYqQ5"
			"gvTatBy8Ihk288GKN+S0/+STvzGaEisrwYEYFSh0Mos3AOaCdZxKhUadzfqT5mNc9NlyFg8AXp3O"
			"bqaV0Po6wmZUX8t3ZUIDRk6UTQcix10h9lsuvXWW+UJTWE5qrFh8usrrs1/0+IyVpjURFWafzwvC"
			"fdHjDL2YiK0QKJEsb4aEEA38qKMNrTWgdEEhhgYSZQTmn5QJr5cJrpxHd1pDM1VXjiEvp4Q2TiSm"
			"LwxenQ5VkH3sN8y1gm0dw4ZUi2nmhobGwGXw7rtttsk+57/n4WwdfM7ku2bijbD4t9lWcyvZ07ef"
			"v2tlrH5rztkt4+L6QW+XkHg7dUbWsw/9SC0NsnxkZGRqIbKNVBoArl0ecvWInSe5r7F9bjfXKHpu"
			"3PPFRV4rofUmUXVOHxpKfFrAPaoi1P6emth+Y4mNN2MN5ByVJDOPubgwe4Tl/+xZw/MmT1bcjyOu"
			"d12+83wnzM/dtve0tIGz2akwxuvrVXMa3ZSEuwh1/3qq6/Aq38/+XlpXdD6+Veya0Giu3FcTG9o6"
			"6i/zpLeXn2IjO4DZ783wen/kyq0ESNGWHU5IkkRsqoCulXuq7oQjcXOjeJFg2kOnEIYv8QI+Ar72"
			"tgjae/DQL8QdC65VpanNSAGIhDSoFmcwCmEBmwf7dSS0GpJ2+hubNltP/BJBFp5pMzE5bI9ZYpzn"
			"Rc+L2+vf//gn1BcXKJ4XkBAIOwovaa4DFveAp87lFckEb65tupN2X0xv+kj2WbuuEoIrgEOV7TIk"
			"hnJu2bosGEmiiZhHUcz2GocIvHUMeTJJ1Q7FolA74bKJASY6aHtRP1tQUbF5MqEpku4LaGZO8zlk"
			"yZNkmw/eOigsKAv2S9J+BHNmwfcIHu9l3KyobyiXPa8zmS1gydk1pCdCx2Xm2XV2ZRH+yqeu3fXV"
			"ck2JGysp4YPcyiDodASSxmDQYZyo3SoFwPbkiUycRXlZHtPJTQaz4YzCc3tXfVuS2hEvZQPHz68X"
			"i445L333lOMVixc92d0xUjOaW83Q1paGixd/pLiuX1iJyqWB92MpnmLZANBEEkL+kP/AC/ZfV8ca"
			"ZV1NyYkJGC+RRo+zCyEVqtRvqFdAkMM3CgdTmo4hsEyHlYsE8jDCU0dwdgEUQ3SEmidreNTnNCcb"
			"61FD5FOvyc2b94MjQ0Nsv7DYUOHQnJKkuIl/7K9GGIIcVWUl0vO9Xs2RaoUfzwvLK2FK4LIKTNE9"
			"NQ+wtiZEt5dLz2PkDWMGD789MiRI6vpmoJqgrVAaGKHZdDIyGmUXBxlZ1DHV9i3C6llAduXGfZ4W"
			"BOxuLlTHeh2nAH0kZUe4ar3dnNDzZs1a5ZkO0Y7biKkpqS0FmQsgphfjle87k8/1kH600/DveMjI"
			"WPhs1BOyA2jJ3FtR3Qrohq7smsPNOW+Cab0xRY/BjoywhfmZfsp7kd19uURaXRjEqkXgzqjfaiNW"
			"hML5XGhMgrRY43SSWAZTzOSl55VntIw4goVtAsuG8D6km9agWVM0yMWjHZrQhs1evQpPz+/imnx8"
			"ZJ9a9euFeal4adX5NKfXi+AyPFjLV6/seES/OPvH8PT8+fD/Q88cK2nT3pGUGOX334HWg4etXj+4"
			"KG9YOHqAIvHbFlzWbIN3ajx8fH1pJx5WrtOxDESo63XYTzeSElofPzMfi9/rONpGCQyw4OaiOVlK"
			"tPeKk38nhjL/DOu0NzDUCEnPf1p2X0vLlli8QsvG9ZtggcT7rdLudDN+IetAeAfoBwIgq5GubGz9"
			"HnpWL68u8aMMdnBKEPBtiWv1OQbqrGbk0MdqbGOpxV4SpCIuFoFyxfgC+xIZnh9FoWcS7MOIic0F"
			"8CYu8eUDxw4cMe8efMk+06ePAm5q3MtLmm1IucVWJ27Gvr2661Jefr7e8GC1f7X3YxmuCrMM+sJd"
			"DW+u0E6djZw4ABBnaE7VUWHRTuO5gyFRtMxcpej/UmNZTwNX0A8KUiEhexnEOLR3C1Lrol2sgfYV"
			"iXxmO/TeXuEkUJHxsyZMxsIsUn2bdu2DXa8v0NY4Ffp3/0P3A+ffXoQ4hN+Db16+6gqAxLZ9Kf6w"
			"RulwZIAkJ6k9tNZM6xZ1CR7neXLV2CZcu4Ou9uksjmsvWXOAO1dfzREZG08wFkTqoWvPjswFevAZ"
			"FjG0+yqRlxYpQnPNCGgPSIJaUFm6GY8DGwTuLNc+JMxzPD1hEqGhYWdqj569LVlS5e+NDc1VbI/O"
			"ztbWJMxXmasrSuWZq8QEob9by/eBKe/+R5aW9oskhiS16gIX4iWmTQtR2pNjVcImTXC5Sbp0l2El"
			"MEYEVFB6pNnQ3OUWVFDmkcaiiuV1FoxRGvhx55ilE53DeF4GrlfOJ5GS64zcNK1hyyHlcXQLp3P9"
			"WFCRmWdL3aEYKjaSSTCTrtQo/bQZgrBg+DrKRUNHzs2hxhcbFpqakxBofQev5STI6w2Ff/gg1avh"
			"fO/ErpMB0CCu9zcDLs/fh0eTu0nbAsZ6qs4r0wJP5xtJmR2SXbcbOiwIfC7hQvR1TjHxqawdRyqz"
			"IZ8Q51YbleBLogbx9MISeUCfSQdTrquVfqkkxuptBJCNCzt0pXY4kSiclSEaKKnjJ11wtuTKkuML"
			"SE1NbU+NiZGdn8OITV0QdJ8LqZrQnKb+dhjkL0ukKixXkJiJTNcSf+FJ+oJmbVK9vn794c1f1yDe"
			"c0ZGxFxykmEZGuHW+XkcnOFxm4vWYz3rYByPU5XJ7Us0P8LVpYnTKT2aEKLMBpNqNKys7MbcAK1H"
			"F5dswYWLFx47WOgjMS2btklVeXCaMYXZtXLuhn9/PtCYcF6GDhw4FJS/lINOqk6G41R7bm2EJInT"
			"ajW2xs1y3iaJwWJTNYxqWXRujU5obk4jOPGVfv7B8zOz18HQxVIraKiAh6Mj4f3398JZnMHddq7+"
			"ypTWTCSEVWZXDTjNTLrAwX5G2DIkKGbSLlzNGwGW8hFlRFTRlhqrQo5odn+AsQ6Pw3Hfdw+SARde"
			"fi9LtBfBGGip5KZRxIa4p7Ie0oDAvyfXJef3xARESF7TFNTEyxbvgymTYuHXbt2wrXvuVhLdED3I"
			"qq5FemNsuNlCH9/P4HMhg0buomUd47GTaCWIOrEDs7R+XpSuL7uyFvF/LQZnrIyPyGPREYVay+gj"
			"dyJY3zgwfD21IrfExlZ5O/vH5O/dm391KlTFY87e+4cIbblED9tGhSuXy/MXVNyOW4/cIPidS43d"
			"ggr5i9PbxDci5bUHCrHrVu24u9SUs45dqi+M9yGtpzvSWMBuhzEVzGehkEicZ5ww5BESELjd8b34"
			"qpEVTbZk8LzleDryZWPvPfe6s8+/TT2xd//vig6Kips+YoVgjKTwzlCbBs2bBDSgAEDAJUdjsN1u"
			"i2HDRsGH354RfgEDCqwS01mOP+9GU6R/3+psFq+HB5JTISUlJSGgICA2aR8pfaoN7r/SGdD+9FCL"
			"YnFWUTqMtD5x0tRiRxmeG5wPC2R1Mkj7p/o6ssV54uh6zXVjioeQ7XLPDHwwxK8UFl4Oj7dvz+Y/"
			"OScPXv2ubzXX4fKykqHlwFJ8sUXX4SIsWNryJ8J4ydMOMUfTw4O1wchuK7uV3wZoHXH9pxHVkUIj"
			"K+WwwmNDvv37YslP0VHjx4dvHHjRjhaXW33PP38/SAlOQWVWT2S6oSJE/P4neDg4ODghKYJ9n2yb"
			"w6SCxJbcXExVH6ivWJDRZacnAzRUdHg7+//OuY38b6JJt76HBwcHJzQNMcnlZVIbBnnzp0L21tZK"
			"bgiq21QbbcMuBmi74uGKVOm4NjbaVSDJOXdFxXFiYyDg4ODE5r9Ubl3bwj5QXLD9a7C0BVZffQoN"
			"DY1wddffy0c0yT+f8Att8AAcRFkorxg6JAhMIQkDB4hqgxJrJyk0qjo6FLeshwcHByc0JyGvRUVG"
			"EASTlIsSSFiQuD2MJJwLKxTxpnE/wspOibmFG9BDg4ODk5oHBwcHBwcivDmTcDBwcHB4Q74fwEGA"
"PNNNssrHc8WAAAAAElFTkSuQmCC\">");
}

void
report_formatter_html::add_header()
{
	add_exact("<header id=\"main_header\">\n");
}

void
report_formatter_html::end_header()
{
	add_exact("</header>\n\n");
}

void
report_formatter_html::add_div(struct tag_attr * div_attr)
{
	string empty="";
	string tmp_str;

	if (div_attr->css_class == empty && div_attr->css_id == empty)
		add_exact("<div>\n");

	else if (div_attr->css_class == empty && div_attr->css_id != empty)
		addf_exact("<div id=\"%s\">\n", div_attr->css_id);

	else if (div_attr->css_class != empty && div_attr->css_id == empty)
		addf_exact("<div class=\"%s\">\n", div_attr->css_class);

	else if (div_attr->css_class != empty && div_attr->css_id != empty)
		addf_exact("<div class=\"%s\" id=\"%s\">\n",
				div_attr->css_class, div_attr->css_id);
}

void
report_formatter_html::end_div()
{
	add_exact("</div>\n");
}

void
report_formatter_html::add_title(struct tag_attr *title_att, const char *title)
{
	addf_exact("<h2 class=\"%s\"> %s </h2>\n", title_att->css_class, title);
}

void
report_formatter_html::add_navigation()
{
	add_exact("<br/><div id=\"main_menu\"> </div>\n");
}

void
report_formatter_html::add_summary_list(string *list, int size)
{
	int i;
	add_exact("<div><br/> <ul>\n");
	for (i=0; i < size; i+=2){
		addf_exact("<li class=\"summary_list\"> <b> %s </b> %s </li>",
				list[i].c_str(), list[i+1].c_str());
	}
	add_exact("</ul> </div> <br />\n");
}


void
report_formatter_html::add_table(string *system_data, struct table_attributes* tb_attr)
{
	int i, j;
	int offset=0;
	string  empty="";

	if (tb_attr->table_class == empty)
		add_exact("<table>\n");
	else
		addf_exact("<table class=\"%s\">\n", tb_attr->table_class);

	for (i=0; i < tb_attr->rows; i++){
		if (tb_attr->tr_class == empty)
			add_exact("<tr> ");
		else
			addf_exact("<tr class=\"%s\"> ", tb_attr->tr_class);

		for (j=0; j < tb_attr->cols; j++){
			offset = i * (tb_attr->cols) + j;

			if (tb_attr->pos_table_title == T &&  i==0)
				addf_exact("<th class=\"%s\"> %s </th> ",
				tb_attr->th_class,system_data[offset].c_str());
			else if (tb_attr->pos_table_title == L &&  j==0)
				addf_exact("<th class=\"%s\"> %s </th> ",
				tb_attr->th_class, system_data[offset].c_str());
			else if (tb_attr->pos_table_title == TL && ( i==0 || j==0 ))
				addf_exact("<th class=\"%s\"> %s </th> ",
				tb_attr->th_class, system_data[offset].c_str());
			else if (tb_attr->pos_table_title == TC && ((i % tb_attr->title_mod ) == 0))
				addf_exact("<th class=\"%s\"> %s </th> ", tb_attr->th_class,
					system_data[offset].c_str());
			else if (tb_attr->pos_table_title == TLC && ((i % tb_attr->title_mod) == 0 || j==0))
				addf_exact("<th class=\"%s\"> %s </th> ", tb_attr->th_class,
				system_data[offset].c_str());
			else
				if ( tb_attr->td_class == empty)
					addf_exact("<td > %s </td> ", system_data[offset].c_str());
				else
					addf_exact("<td class=\"%s\"> %s </td> ", tb_attr->td_class,
						system_data[offset].c_str());
		}
		add_exact("</tr>\n");
	}
	add_exact("</table>\n");
}

