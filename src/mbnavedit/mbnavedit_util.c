/*
 * Description: This module contains various utilities, converters
 *	for XmStrings (to work properly with app-defaults), and if needed
 *	the XPM pixmap utilities.
 */

#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <mb_config.h>

/* Need to include windows.h BEFORE the the Xm stuff otherwise VC14+ barf with conflicts */
#if defined(_MSC_VER) && (_MSC_VER >= 1900)
#	ifndef WIN32
#		define WIN32
#	endif
#	include <WinSock2.h>
#include <windows.h>
#endif

#include <Xm/RowColumn.h>
#include <Xm/Xm.h>

// Handy definition used in SET_BACKGROUND_COLOR
#define UNSET (-1)

#define XTPOINTER XtPointer

/*
 * The following enum is used to support wide character sets.
 * Use this enum for references into the Common Wide Characters array.
 * If you add to the array, ALWAYS keep NUM_COMMON_WCHARS as the last
 * entry in the enum.  This will maintain correct memory usage, etc.
 */
enum {
	WNull,
	WTab,
	WNewLine,
	WCarriageReturn,
	WFormFeed,
	WVerticalTab,
	WBackSlash,
	WQuote,
	WHash,
	WColon,
	WideF,
	WideL,
	WideN,
	WideR,
	WideT,
	WideV,
	WideUF,
	WideUL,
	WideUR,
	WideUT,
	WideZero,
	WideOne,
	NUM_COMMON_WCHARS
};

/*
 * Function:
 *      len = strlenWc(ptr);
 * Description:
 *      Return the number of characters in a wide character string (not
 *	the characters in the resultant mbs).
 * Input:
 *      ptr - wchar_t* : pointer to the wcs to count
 * Output:
 *      int : the number of characters found
 */
static int strlenWc(wchar_t *ptr) {
	if (!ptr)
		return (0);

	wchar_t *p = ptr;
	int x = 0;

	while (*p++)
		x++;
	return (x);
}

/*
 * Function:
 *      bytesConv = doMbstowcs(wcs, mbs, n);
 * Description:
 *      Create a wcs string from an input mbs.
 * Input:
 *	wcs - wchar_t* : pointer to result buffer of wcs
 *      mbs - char* : pointer to the source mbs
 *	n - size_t : the number of characters to convert
 * Output:
 *      bytesConv - size_t : number of bytes converted
 */
static size_t doMbstowcs(wchar_t *wcs, char *mbs, size_t n) {
	return (mbstowcs(wcs, mbs, n));
}

/*
 * Function:
 *      bytesConv = doWcstombs(wcs, mbs, n);
 * Description:
 *      Create a mbs string from an input wcs.
 * Input:
 *	wcs - wchar_t* : pointer to the source wcs
 *      mbs - char* : pointer to result mbs buffer
 *	n - size_t : the number of characters to convert
 * Output:
 *      bytesConv - size_t : number of bytes converted
 */
static size_t doWcstombs(char *mbs, wchar_t *wcs, size_t n) {
	const size_t retval = wcstombs(mbs, wcs, (n * sizeof(wchar_t)));
	if (retval == (size_t)-1)
		return (0);
	else
		return (retval);
}

/*
 * Function:
 *      status = dombtowc(wide, multi, size);
 * Description:
 *      Convert a multibyte character to a wide character.
 * Input:
 *      wide	- wchar_t *	: where to put the wide character
 *	multi	- char *	: the multibyte character to convert
 *	size	- size_t	: the number of characters to convert
 * Output:
 *      0	- if multi is a NULL pointer or points to a NULL character
 *	#bytes	- number of bytes in the multibyte character
 *	-1	- multi is an invalid multibyte character.
 *
 *	NOTE:  if wide is NULL, then this returns the number of bytes in
 *	       the multibyte character.
 */
static int dombtowc(wchar_t *wide, char *multi, size_t size) {
	return mbtowc(wide, multi, size);
}

/*
 * Function:
 *      cwc = CStrCommonWideCharsGet();
 * Description:
 *      Return the array of common wide characters.
 * Input:
 *      None.
 * Output:
 *     	cwc - wchar_t * : this array should never be written to or FREEd.
 */
static wchar_t *CStrCommonWideCharsGet() {
	static wchar_t *CommonWideChars = NULL;
	/*
	 * If you add to this array, don't forget to change the enum in
	 * the TYPEDEFS and DEFINES section above to correspond to this
	 * array.
	 */
	static char *characters[] = {"\000", "\t", "\n", "\r", "\f", "\v", "\\", "\"", "#", ":", "f",
	                             "l",    "n",  "r",  "t",  "v",  "F",  "L",  "R",  "T", "0", "1"};

	if (CommonWideChars == NULL) {
		// Allocate and create the array.
		CommonWideChars = (wchar_t *)XtMalloc(NUM_COMMON_WCHARS * sizeof(wchar_t));

		for (int i = 0; i < NUM_COMMON_WCHARS; i++) {
			(void)dombtowc(&(CommonWideChars[i]), characters[i], 1);
		}
	}
	return (CommonWideChars);
}

/*
 * Function:
 *      copyWcsToMbs(mbs, wcs, len);
 * Description:
 *      Create a mbs string from an input wcs. This function allocates
 *	a buffer if necessary.
 * Input:
 *	mbs - char* : destination for the converted/copied output
 *	wcs - wchar_t* : pointer to wcs to copy/convert
 *	len - int : the number of wchar_t' to convert
 *	process_it - Boolean : True if processing of quoted charcters,
 *			False if blind.
 * Output:
 *      None
 */
static void copyWcsToMbs(char *mbs, wchar_t *wcs, int len, Boolean process_it) {
	static wchar_t *tbuf = NULL;
	static int tbufSize = 0;

	// Make sure there's room in the buffer
	if (tbufSize < len) {
		tbuf = (wchar_t *)XtRealloc((char *)tbuf, (len + 1) * sizeof(wchar_t));
		tbufSize = len;
	}

	wchar_t *fromP = wcs;
	wchar_t *x = &fromP[len];
	wchar_t *commonWChars = CStrCommonWideCharsGet();

	// Now copy and process
	wchar_t *toP = tbuf;
	int lenToConvert = 0;
	while (fromP < x) {
		// Check for quoted characters
		if ((*fromP == commonWChars[WBackSlash]) && process_it) {
			fromP++;        /* Skip quote */
			if (fromP == x) /* Hanging quote? */
			{
				*toP++ = commonWChars[WBackSlash];
				lenToConvert++;
				break;
			}
			const wchar_t tmp = *fromP++;
			if (tmp == commonWChars[WideN]) {
				*toP++ = commonWChars[WNewLine];
			}
			else if (tmp == commonWChars[WideT]) {
				*toP++ = commonWChars[WTab];
			}
			else if (tmp == commonWChars[WideR]) {
				*toP++ = commonWChars[WCarriageReturn];
			}
			else if (tmp == commonWChars[WideF]) {
				*toP++ = commonWChars[WFormFeed];
			}
			else if (tmp == commonWChars[WideV]) {
				*toP++ = commonWChars[WVerticalTab];
			}
			else if (tmp == commonWChars[WBackSlash]) {
				*toP++ = commonWChars[WBackSlash];
			}
			else {
				/*
				 * No special translation needed
				 */
				*toP++ = tmp;
			}
		}
		else {
			*toP++ = *fromP++;
		}
		lenToConvert++;
	}

	wchar_t tmp = tbuf[lenToConvert];
	tbuf[lenToConvert] = (wchar_t)0;
	const int numCvt = doWcstombs(mbs, tbuf, lenToConvert);
	tbuf[lenToConvert] = tmp;

	mbs[numCvt] = '\0';
}

/*
 * Function:
 *	ptr = getNextSepartor(str);
 * Description:
 *	Parse through a string looking for the next compound string
 *	field separator
 * Inputs:
 *	str - wchar_t* : the address of address of the string to parse
 * Outputs:
 *	ptr - wchar_t* : pointer to character, if found, points to end
 *			of string otherwise ('\0').
 */
static wchar_t *getNextSeparator(wchar_t *str) {
	wchar_t *ptr = str;
	wchar_t *commonWChars = CStrCommonWideCharsGet();

	while (*ptr) {
		/*
		 * Check for separator
		 */
		if ((*ptr == commonWChars[WHash]) || (*ptr == commonWChars[WQuote]) || (*ptr == commonWChars[WColon])) {
			return (ptr);
		}
		else if (*ptr == commonWChars[WBackSlash]) {
			ptr++;
			if (*ptr)
				ptr++; /* Skip quoted character */
		}
		else {
			ptr++;
		}
	}
	return (ptr);
}

/*
 * Function:
 *	more =
 *        extractSegment(str, tagStart, tagLen, txtStart, txtLen,
 *			pDir, pSep);
 * Description:
 *	Parse through a string version of a compound string and extract
 *	the first compound string segment from the string.
 * Inputs:
 *	str - char** : the address of address of the string to parse
 *	tagStart - char** : address to return pointer to tag start into
 *	tagLen - int* : address where to return the tag length into
 *	txtStart - char** : address to return the text start into
 *	txtLen - int* : address where to return the text length
 *	pDir - int* : address to return the string direction into
 *	pSep - Boolean * : address to return the separtor into
 * Outputs:
 *	more - Boolean : True if more of the string to parse.
 *			False means done.
 */
static Boolean extractSegment(
    wchar_t **str, wchar_t **tagStart, int *tagLen, wchar_t **txtStart,
    int *txtLen, int *pDir, Boolean * pSep) {
	// Guard against nulls
	wchar_t *start = *str;
	wchar_t emptyStrWcs[1];
	wchar_t *commonWChars = CStrCommonWideCharsGet();
	if (!start) {
		start = emptyStrWcs;
		emptyStrWcs[0] = commonWChars[WNull];
	}

	wchar_t *text = NULL;
	int textL = 0;
	bool tagSeen = False;
	wchar_t *tag = NULL;
	int tagL = 0;
	bool modsSeen = False;
	int dir = XmSTRING_DIRECTION_L_TO_R;
	bool sep = False;
	bool done = False;
	/*
	 * If the first character of the string isn't a # or a ", then we
	 * just have a regular old simple string. Do the same the thing for
	 * the empty string.
	 */
	if ((*start == '\0') || (start != getNextSeparator(start))) {
		text = start;
		if (!(textL = strlenWc(start))) {
			text = NULL;
		}
		start += textL;
	} else {
		done = False;
		while (!done) {
			if (*start == commonWChars[WHash]) {
				if (tagSeen) {
					done = True;
					break;
				}
				else {
					tagSeen = True;
					tag = ++start;
					start = getNextSeparator(tag);
					if ((tagL = start - tag) == 0) {
						tag = NULL; /* Null tag specified */
					}
				}
			}
			else if (*start == commonWChars[WQuote]) {
				text = ++start;
				start = getNextSeparator(start);
				while (!((*start == commonWChars[WQuote]) || (*start == commonWChars[WNull]))) {
					start = getNextSeparator(++start);
				}

				if ((textL = start - text) == 0) {
					text = NULL; /* Null text specified  */
				}
				/*
				 * if a quote, skip over it
				 */
				if (*start == commonWChars[WQuote]) {
					start++;
				}
				done = True;
			}
			else if (*start == commonWChars[WColon]) {
				if (modsSeen) {
					done = True;
					break;
				}

				/*
				 * If the next character is a t or f, the we've got
				 * a separator.
				 */
				modsSeen = True;
				bool checkDir = False;
				start++;
				if ((*start == commonWChars[WideT]) || (*start == commonWChars[WideUT]) || (*start == commonWChars[WideOne])) {
					sep = True;
					start++;
					checkDir = True;
				}
				else if ((*start == commonWChars[WideF]) || (*start == commonWChars[WideUF]) ||
				         (*start == commonWChars[WideZero])) {
					sep = False;
					start++;
					checkDir = True;
				}
				else if ((*start == commonWChars[WideR]) || (*start == commonWChars[WideUR])) {
					start++;
					dir = XmSTRING_DIRECTION_R_TO_L;
				}
				else if ((*start == commonWChars[WideL]) || (*start == commonWChars[WideUL])) {
					start++;
					dir = XmSTRING_DIRECTION_L_TO_R;
				}
				/*
				 * Look for direction if necessary. This requires a bit of
				 * look ahead.
				 */
				if (checkDir && (*start == commonWChars[WColon])) {
					if ((*(start + 1) == commonWChars[WideL]) || (*(start + 1) == commonWChars[WideUL])) {
						dir = XmSTRING_DIRECTION_L_TO_R;
						start += 2;
					}
					else if ((*(start + 1) == commonWChars[WideR]) || (*(start + 1) == commonWChars[WideUR])) {
						dir = XmSTRING_DIRECTION_R_TO_L;
						start += 2;
					}
				}
			}
			else {
				/*
				 * A bad string format! We'll just skip the character.
				 */
				start++;
			}
		}
	}

	/*
	 * Now fill in return values
	 */
	if (*str)
		*str = start;
	if (tagStart)
		*tagStart = tag;
	if (tagLen)
		*tagLen = tagL;
	if (txtStart)
		*txtStart = text;
	if (txtLen)
		*txtLen = textL;
	if (pDir)
		*pDir = dir;
	if (pSep)
		*pSep = sep;

	return *start != commonWChars[WNull];
}

/*
 * Function:
 *	xstr = StringToXmString(str);
 * Description:
 *	Parse a string into an XmString.
 * Inputs:
 *	str - char * : the string to parse
 * Outputs:
 *	xstr - XmString : the allocated return structure
 */
static XmString StringToXmString(char * str) {
	if (!str)
		return (NULL);

	static char *tagBuf = NULL;
	static int tagBufLen = 0;
	static char *textBuf = NULL;
	static int textBufLen = 0;

	/*
	 * For expediencies sake, we'll overallocate this buffer so that
	 * the wcs is guaranteed to fit (1 wc per byte in original string).
	 */
	wchar_t *wcStr = (wchar_t *)XtMalloc((strlen(str) + 1) * sizeof(wchar_t));
	doMbstowcs(wcStr, str, strlen(str) + 1);

	/*
	 * Create the beginning segment
	 */
	int curDir = XmSTRING_DIRECTION_L_TO_R;
	XmString xmStr = XmStringDirectionCreate(curDir);

	wchar_t *tag;
	int tagLen;
	wchar_t *text;
	int textLen;
	Boolean sep;
	int dir;

	// Convert the string.
	bool more = True;
	wchar_t *ctx = wcStr;
	while (more) {
		more = extractSegment(&ctx, &tag, &tagLen, &text, &textLen, &dir, &sep);
		// Pick up a direction change
		if (dir != curDir) {
			curDir = dir;
			XmString s1 = XmStringDirectionCreate(curDir);
			XmString s2 = xmStr;
			xmStr = XmStringConcat(s2, s1);
			XmStringFree(s1);
			XmStringFree(s2);
		}

		/*
		 * Create the segment. Text and tag first.
		 */
		if (textLen) {
			if (textBufLen <= (textLen * sizeof(wchar_t))) {
				textBufLen = (textLen + 1) * sizeof(wchar_t);
				textBuf = (char *)XtRealloc(textBuf, textBufLen);
			}
			copyWcsToMbs(textBuf, text, textLen, True);

			if (tagLen) {
				if (tagBufLen <= (tagLen * sizeof(wchar_t))) {
					tagBufLen = (tagLen + 1) * sizeof(wchar_t);
					tagBuf = (char *)XtRealloc(tagBuf, tagBufLen);
				}
				copyWcsToMbs(tagBuf, tag, tagLen, False);
			}
			else {
				if (!tagBuf) {
					tagBufLen = strlen(XmSTRING_DEFAULT_CHARSET) + 1;
					tagBuf = (char *)XtMalloc(tagBufLen);
				}
				strcpy(tagBuf, XmSTRING_DEFAULT_CHARSET);
			}

			XmString s1 = XmStringCreate(textBuf, tagBuf);
			XmString s2 = xmStr;
			xmStr = XmStringConcat(s2, s1);
			XmStringFree(s1);
			XmStringFree(s2);
		}

		/*
		 * Add in the separators.
		 */
		if (sep) {
			XmString s1 = XmStringSeparatorCreate();
			XmString s2 = xmStr;
			xmStr = XmStringConcat(s2, s1);
			XmStringFree(s1);
			XmStringFree(s2);
		}
	}

	XtFree((char *)wcStr);
	return (xmStr);
}

/*
 * Function:
 *      nextCStr = getNextCStrDelim(str);
 * Description:
 *      Find the next unquoted , or \n in the string
 * Input:
 *	str - char * : the input string
 * Output:
 *      nextCStr - char* : pointer to the next delimiter. Returns NULL if no
 *			delimiter found.
 */
static char *getNextCStrDelim(char * str) {
	if (!str)
		return (NULL);
	if (!*str)
		return (NULL); /* At end */

	bool inQuotes = false;
	int len = mblen(NULL, sizeof(wchar_t));
	char *comma = str;
	while (*comma) {
		if ((len = mblen(comma, sizeof(wchar_t))) > 1) {
			comma += len;
			continue;
		}

		if (*comma == '\\') {
			comma++; /* Over quote */
			comma += mblen(comma, sizeof(wchar_t));
			continue;
		}

		// See if we have a delimiter
		if (!inQuotes) {
			if ((*comma == ',') || (*comma == '\012')) {
				return (comma);
			}
		}

		// Deal with quotes
		if (*comma == '\"') {
			inQuotes = !inQuotes;
		}

		comma++;
	}

	return (NULL);
}

/*
 * Function:
 *	cnt = getCStrCount(str);
 * Description:
 *      Get the count of cstrings in a compound string table ascii
 *	format.
 * Input:
 *      str - char * : string to parse
 * Output:
 *      cnt - int : the number of XmStrings found
 */
static int getCStrCount(char *str) {
	if (!str)
		return (0);
	if (!*str)
		return (0);

	int x = 1;
	char *newStr;
	while ((newStr = getNextCStrDelim(str))) {
		x++;
		str = ++newStr;
	}
	return (x);
}

/*
 * Function:
 *	CONVERTER CvtStringToXmString
 *
 * Description:
 *	Convert a string to an XmString. This allows a string contained in
 *	resource file to contain multiple fonts. The syntax for the string
 *	is:
 *		::[#[font-tag]]"string"[#[font-tag]"string"] ...
 *
 *	note that the # can be escaped (\#).
 *
 * Input:
 * Output:
 *	Standard.
 */
static Boolean CvtStringToXmString(
    Display *d, XrmValue *args, Cardinal *num_args, XrmValue *fromVal,
    XrmValue *toVal, XtPointer data) {
	(void)args;  // Unused parameter
	(void)data;  // Unused parameter

	// This converter takes no parameters
	if (*num_args != 0) {
		XtAppWarningMsg(XtDisplayToApplicationContext(d), "cvtStringToXmString", "wrongParameters", "XtToolkitError",
		                "String to XmString converter needs no extra arguments", NULL, NULL);
	}

	// See if this is a simple string
	char *str = (char *)fromVal->addr;
	static XmString resStr;
	if (strncmp(str, "::", 2)) {
		resStr = XmStringCreateLtoR(fromVal->addr, XmSTRING_DEFAULT_CHARSET);
	} else {
		// Convert into internal format
		resStr = StringToXmString(fromVal->addr + 2); /* skip :: */
	}

	// Done, return result
	if (toVal->addr == NULL) {
		toVal->addr = (XTPOINTER)&resStr;
		toVal->size = sizeof(XmString);
	}
	else if (toVal->size < sizeof(XmString)) {
		toVal->size = sizeof(XmString);
		XtDisplayStringConversionWarning(d, fromVal->addr, "XmString");
		XmStringFree(resStr);
		return (False);
	}
	else {
		*(XmString *)toVal->addr = resStr;
		toVal->size = sizeof(XmString);
	}
	return (True);
}

/*
 * Function:
 *      CONVERTER CvtStringToXmStringTable
 *
 * Description:
 *	Convert a string to an XmString table. This allows a string
 *	contained in resource file to contain multiple fonts. The syntax
 *	for the string is:
 *
 *	   compound_string = [#[font-tag]]"string"[#[font-tag]"string"] ...
 *	   compound_string_table = [compound_string][,compound_string] ...
 *
 *	note that the # can be escaped (\#).
 *
 * Input:
 * Output:
 *	Standard.
 */
static Boolean CvtStringToXmStringTable(
    Display *d, XrmValue *args, Cardinal *num_args, XrmValue *fromVal,
    XrmValue *toVal, XtPointer data) {
	(void)data;  // Unused parameter
	/*
	 * This converter takes no parameters
	 */
	if (*num_args != 0) {
		XtAppWarningMsg(XtDisplayToApplicationContext(d), "cvtStringToXmStringTable", "wrongParameters", "XtToolkitError",
		                "String to XmStringTable converter needs no extra arguments", NULL, NULL);
	}

	char *str;
	/*
	 * Set str and make sure there's somethin' there
	 */
	if (!(str = (char *)fromVal->addr)) {
		str = "";
	}

	// Allocate the XmStrings + 1 for NULL termination
	static XmString *CStrTable;  // TODO(schwehr): Leaky?
	CStrTable = (XmString *)XtMalloc((getCStrCount(str) + 1) * sizeof(XmString *));

	/*
	 * Use the string converter for the strings
	 */
	char *tmpBuf = (char *)XtMalloc(strlen(str) + 1);
	strcpy(tmpBuf, str);
	str = tmpBuf;

	// Create strings
	XmString *tblPtr = CStrTable;

	if (*str) {
		XrmValue fVal;
		XrmValue tVal;
		while (str) {
			char *nextDelim = getNextCStrDelim(str);

			// Overwrite nextDelim
			if (nextDelim) {
				*nextDelim = '\0';
				nextDelim++;
			}

			// Convert it
			fVal.size = strlen(str) + 1;
			fVal.addr = str;
			tVal.size = sizeof(XTPOINTER);
			tVal.addr = (XTPOINTER)tblPtr;

			/*
			 * Call converter ourselves since this is used to create
			 * the strings in the table we create. We need to do this
			 * since we don't have a widget to send to the XtConvertAndStore
			 * function. Side effects are that we can never get these
			 * compound strings cached and that no destructor function is
			 * called when the strings leave existence, but we nuke 'em
			 * in the XmStringTable destuctor.
			 */
			CvtStringToXmString(d, args, num_args, &fVal, &tVal, NULL);
			tblPtr++;
			str = nextDelim;
		}
	}
	XtFree(tmpBuf);

	// Null terminate
	*tblPtr = NULL;

	// Done, return result
	if (toVal->addr == NULL) {
		toVal->addr = (XTPOINTER)&CStrTable;
		toVal->size = sizeof(XmString);
	}
	else if (toVal->size < sizeof(XmString *)) {
		toVal->size = sizeof(XmString *);
		XtDisplayStringConversionWarning(d, fromVal->addr, "XmStringTable");

		tblPtr = CStrTable;
		while (*tblPtr) {
			XmStringFree(*tblPtr);
		}
		XtFree((char *)CStrTable);
		return (False);
	}
	else {
		*(XmString **)toVal->addr = CStrTable;
		toVal->size = sizeof(XmString *);
	}
	return (True);
}

/*****************************************************************************
 *	GLOBAL CODE
 *****************************************************************************/

/*
 * Function:
 *      RegisterBxConverters(appContext);
 * Description:
 *      This globally available function installs all the converters necessary
 *	to run BuilderXcessory generated interfaces that use compound
 *	strings. This is necessary since Motif has not supplied very smart
 *	converters.
 * Input:
 *      appContext - XtAppContext : the application context
 * Output:
 *      None
 */
void RegisterBxConverters(XtAppContext appContext) {
	XtAppSetTypeConverter(appContext, XmRString, XmRXmString, (XtTypeConverter)CvtStringToXmString, NULL, 0, XtCacheNone, NULL);

	XtAppSetTypeConverter(appContext, XmRString, XmRXmStringTable, (XtTypeConverter)CvtStringToXmStringTable, NULL, 0,
	                      XtCacheNone, NULL);
}

/*
 * Function:
 *      CONVERT(w, from_string, to_type, to_size, success);
 * Description:
 *      A converter wrapper for convenience from BuilderXcessory.
 * Input:
 *      w - Widget : the widget to use for conversion
 *	from_string - char * : the string to convert from
 *	to_type - char * : the type to convert to
 *	to_size - int : the size of the conversion result
 *	success - Boolean* : Set to the result value of the conversion
 * Output:
 *      None
 */
#ifndef IGNORE_CONVERT
XtPointer BX_CONVERT(
    Widget w, char *from_string, char *to_type, int to_size, Boolean *success) {
	(void)to_size;  // Unused parameter
	XtPointer val;           /* Pointer size return value    */

	/*
	 * We will assume that the conversion is going to fail and change this
	 * value later if the conversion is a success.
	 */
	*success = False;

	/*
	 * Since we are converting from a string to some type we need to
	 * set the fromVal structure up with the string information that
	 * the caller passed in.
	 */
	XrmValue fromVal;
	fromVal.size = strlen(from_string) + 1;
	fromVal.addr = from_string;

	/*
	 * Since we are not sure what type and size of data we are going to
	 * get back we will set this up so that the converter will point us
	 * at a block of valid data.
	 */
	XrmValue toVal;
	toVal.size = 0;
	toVal.addr = NULL;

	/*
	 * Now lets try to convert this data by calling this handy-dandy Xt
	 * routine.
	 */
	Boolean convResult;
	convResult = XtConvertAndStore(w, XmRString, &fromVal, to_type, &toVal);

	/*
	 * Now we have two conditions here.  One the conversion was a success
	 * and two the conversion failed.
	 */
	if (!convResult) {
		/*
		 * If this conversion failed that we can pretty much return right
		 * here because there is nothing else we can do.
		 */
		return ((XtPointer)NULL);
	}

	/*
	 * If we get this far that means we did the conversion and all is
	 * well.  Now we have to handle the special cases for type and
	 * size constraints.
	 */
	if (!strcmp(to_type, "String")) {
		/*
		 * Since strings are handled different in Xt we have to deal with
		 * the conversion from a string to a string.  When this happens the
		 * toVal.size will hold the strlen of the string so generic
		 * conversion code can't handle it.  It is possible for a string to
		 * string conversion to happen so we do have to watch for it.
		 */
		val = (XTPOINTER)toVal.addr;
	}
	else if (!strcmp(to_type, "Double")) {
		val = (XTPOINTER)((double *)toVal.addr);
	}
	else if (!strcmp(to_type, "Float")) {
		val = (XTPOINTER)((float *)toVal.addr);
	}
	else {
		/*
		 * Here is the generic conversion return value handler.  This
		 * just does some size specific casting so that value that we
		 * return is in the correct bytes of the XtPointer that we
		 * return.  Here we check all sizes from 1 to 8 bytes.
		 */
		switch (toVal.size) {
		case 1:
			val = (XTPOINTER)(long)(*(char *)toVal.addr);
			break;
		case 2:
			val = (XTPOINTER)(long)(*(short *)toVal.addr);
			break;
		case 4:
			val = (XTPOINTER)(long)(*(int *)toVal.addr);
			break;
		case 8:
		default:
#ifdef _WIN32
			val = (XTPOINTER)(int64_t)(*(int64_t *)toVal.addr);
#else
			val = (XTPOINTER)(long)(*(long *)toVal.addr);
#endif
			break;
		}
	}

	/*
	 * Well everything is done and the conversion was a success so lets
	 * set the success flag to True.
	 */
	*success = convResult;

	/*
	 * Finally lets return the converted value.
	 */
	/*SUPPRESS 80*/
	return (val);
}

#ifdef DEFINE_OLD_BXUTILS
XtPointer CONVERT(
    Widget w, char *from_string, char *to_type, int to_size, Boolean *success) {
	return (BX_CONVERT(w, from_string, to_type, to_size, success));
}
#endif /* DEFINE_OLD_BXUTILS */
#endif /* !IGNORE_CONVERT */

/*
 * Function:
 *      MENU_POST(p, mw, ev, dispatch);
 * Description:
 *      A converter wrapper for convenience from BuilderXcessory.
 * Input:
 *      p - Widget : the widget to post
 *	mw - XtPointer : the menu widget
 *	ev - XEvent* : the event that caused the menu post
 *	dispatch - Boolean* : not used
 * Output:
 *      None
 */

void BX_MENU_POST(Widget p, XtPointer mw, XEvent *ev, Boolean *dispatch) {
	(void)p;  // Unused parameter
	(void)dispatch;  // Unused parameter

	int argcnt = 0;
	Arg args[2];
	int button;
	XtSetArg(args[argcnt], XmNwhichButton, &button);
	argcnt++;
	Widget m = (Widget)mw;
	XtGetValues(m, args, argcnt);
	XButtonEvent *e = (XButtonEvent *)ev;
	if (e->button != button)
		return;
	XmMenuPosition(m, e);
	XtManageChild(m);
}

/*
 * Function:
 *      SET_BACKGROUND_COLOR(w, args, argcnt, bg_color);
 * Description:
 *      Sets the background color and shadows of a widget.
 * Input:
 *      w - The widget to set the background color on.
 *      args, argcnt - The argument list so far.
 *      bg_color - The new background color as a pixel.
 * Output:
 *      none
 *
 *  NOTES:  This assumes that args later in the argument list
 *          override those already in the list.  Therfore i f
 *          there are shadow colors later in the list they will win.
 *
 *          There is no need to use this function when creating a widget
 *          only when doing a set values, shadow colors are automatically
 *          calculated at creation time.
 */
void BX_SET_BACKGROUND_COLOR(Widget w, ArgList args, Cardinal *argcnt, Pixel bg_color) {
#if ((XmVERSION == 1) && (XmREVISION > 0))
#error "Old Xm version"
	/*
	 * Walk through the arglist to see if the user set the top or
	 * bottom shadow colors.
	 */
	selectLoc = topShadowLoc = bottomShadowLoc = UNSET;
	for (i = 0; i < *argcnt; i++) {
		if ((strcmp(args[i].name, XmNtopShadowColor) == 0) || (strcmp(args[i].name, XmNtopShadowPixmap) == 0)) {
			topShadowLoc = i;
		}
		else if ((strcmp(args[i].name, XmNbottomShadowColor) == 0) || (strcmp(args[i].name, XmNbottomShadowPixmap) == 0)) {
			bottomShadowLoc = i;
		}
		else if (strcmp(args[i].name, XmNarmColor) == 0) {
			selectLoc = i;
		}
		else if (strcmp(args[i].name, XmNforeground) == 0) {
			fgLoc = i;
		}
	}

	/*
	 * If either the top or bottom shadow are not set then we
	 * need to use XmGetColors to get the shadow colors from the backgound
	 * color and add those that are not already in the arglist to the
	 * arglist.
	 *
	 */
	if ((bottomShadowLoc == UNSET) || (topShadowLoc == UNSET) || (selectLoc == UNSET) || (fgLoc == UNSET)) {
		Arg large[1];
		Colormap cmap;
		Pixel topShadow;
		Pixel bottomShadow;
		Pixel select;
		Pixel fgColor;

		XtSetArg(large[0], XmNcolormap, &cmap);
		XtGetValues(w, large, 1);
		XmGetColors(XtScreen(w), cmap, bg_color, &fgColor, &topShadow, &bottomShadow, &select);

		if (topShadowLoc == UNSET) {
			XtSetArg(args[*argcnt], XmNtopShadowColor, topShadow);
			(*argcnt)++;
		}

		if (bottomShadowLoc == UNSET) {
			XtSetArg(args[*argcnt], XmNbottomShadowColor, bottomShadow);
			(*argcnt)++;
		}

		if (selectLoc == UNSET) {
			XtSetArg(args[*argcnt], XmNarmColor, select);
			(*argcnt)++;
		}

		if (fgLoc == UNSET) {
			XtSetArg(args[*argcnt], XmNforeground, fgColor);
			(*argcnt)++;
		}
	}
#else
	(void)w;  // Unused parameter
#endif

	XtSetArg(args[*argcnt], XmNbackground, bg_color);
	(*argcnt)++;
}

/*
 * Function:
 *	w = BxFindTopShell(start);
 * Description:
 *	Go up the hierarhcy until we find a shell widget.
 * Input:
 *      start - Widget : the widget to start with.
 * Output:
 *	w - Widget : the shell widget.
 */
#ifndef _BX_FIND_TOP_SHELL
#define _BX_FIND_TOP_SHELL

Widget BxFindTopShell(Widget start) {
	Widget p;

	while ((p = XtParent(start))) {
		start = p;
	}
	return (start);
}
#endif /* _BX_FIND_TOP_SHELL */

/*
 * Function:
 *	BxWidgetIdsFromNames(ref, cbName, stringList)
 * Description:
 *	Return an array of widget ids from a list of widget names.
 * Input:
 *	ref - Widget : reference widget.
 *	cbName - char* : callback name.
 *	stringList - char*: list of widget names.
 * Output:
 *	WidgetList : array of widget IDs.
 */

#ifndef _BX_WIDGETIDS_FROM_NAMES
#define _BX_WIDGETIDS_FROM_NAMES

WidgetList BxWidgetIdsFromNames(Widget ref, char *cbName, char *stringList) {
	/*
	 * For backward compatibility, remove [ and ] from the list.
	 */
	String start = XtNewString(stringList);
	String tmp = start;
	if ((start = strchr(start, '[')) != NULL)
		start++;
	else
		start = tmp;

	while ((start && *start) && isspace(*start)) {
		start++;
	}
	char *ptr = strrchr(start, ']');
	if (ptr) {
		*ptr = '\0';
	}

	WidgetList wgtIds = NULL;
	int wgtCount = 0;
	Widget inst;
	Widget current;
	String widget;
	ptr = start + strlen(start) - 1;
	while (ptr && *ptr) {
		if (isspace(*ptr)) {
			ptr--;
		}
		else {
			ptr++;
			break;
		}
	}
	if (ptr && *ptr) {
		*ptr = '\0';
	}

	/*
	 * start now points to the first character after the [.
	 * the list is now either empty, one, or more widget
	 * instance names.
	 */
	start = strtok(start, ",");
	while (start) {
		while ((start && *start) && isspace(*start)) {
			start++;
		}
		ptr = start + strlen(start) - 1;
		while (ptr && *ptr) {
			if (isspace(*ptr)) {
				ptr--;
			}
			else {
				ptr++;
				break;
			}
		}
		if (ptr && *ptr) {
			*ptr = '\0';
		}

		/*
		 * Form a string to use with XtNameToWidget().
		 */
		widget = (char *)XtMalloc((strlen(start) + 2) * sizeof(char));
		sprintf(widget, "*%s", start);

		/*
		 * Start at this level and continue up until the widget is found
		 * or until the top of the hierarchy is reached.
		 */
		current = ref;
		while (current != NULL) {
			inst = XtNameToWidget(current, widget);
			if (inst != NULL) {
				wgtCount++;
				wgtIds = (WidgetList)XtRealloc((char *)wgtIds, wgtCount * sizeof(Widget));
				wgtIds[wgtCount - 1] = inst;
				break;
			}
			current = XtParent(current);
		}

		if (current == NULL) {
			printf("Callback Error (%s):\n\t\
Cannot find widget %s\n",
			       cbName, widget);
		}
		XtFree(widget);
		start = strtok(NULL, ",");
	}

	/*
	 * NULL terminate the list.
	 */
	wgtIds = (WidgetList)XtRealloc((char *)wgtIds, (wgtCount + 1) * sizeof(Widget));
	wgtIds[wgtCount] = NULL;

	XtFree((char *)tmp);
	return (wgtIds);
}
#endif /* _BX_WIDGETIDS_FROM_NAMES */

XtPointer BX_SINGLE(float val) {
	XtPointer pointer;

	pointer = (XtPointer)XtMalloc(sizeof(float));
	if (pointer != NULL)
		*((float *)pointer) = val;
	return (pointer);
}

#ifdef DEFINE_OLD_BXUTILS
XtPointer SINGLE(float val) { return (BX_SINGLE(val)); }
#endif /* DEFINE_OLD_BXUTILS */

XtPointer BX_DOUBLE(double val) {
	XtPointer pointer;

	pointer = (XtPointer)XtMalloc(sizeof(double));
	if (pointer != NULL)
		*((double *)pointer) = val;
	return (pointer);
}

#ifdef DEFINE_OLD_BXUTILS
XtPointer DOUBLE(double val) { return (BX_DOUBLE(val)); }
#endif /* DEFINE_OLD_BXUTILS */

/****************************************************************************
 *
 * Big chunk of code inserted from Bull (based on modified 3.3)
 *
 ****************************************************************************/

#ifndef IGNORE_XPM_PIXMAP

#ifndef USE_XPM_LIBRARY

#ifdef SYSV
#include <memory.h>
#endif

/*
 * Copyright 1990, 1991 GROUPE BULL
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of GROUPE BULL not be used in advertising
 * or publicity pertaining to distribution of the software without specific,
 * written prior permission.  GROUPE BULL makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * GROUPE BULL disclaims all warranties with regard to this software,
 * including all implied warranties of merchantability and fitness,
 * in no event shall GROUPE BULL be liable for any special,
 * indirect or consequential damages or any damages
 * whatsoever resulting from loss of use, data or profits,
 * whether in an action of contract, negligence or other tortious
 * action, arising out of or in connection with the use
 * or performance of this software.
 *
 */

/* Return ErrorStatus codes:
 * null     if full success
 * positive if partial success
 * negative if failure
 */

#define BxXpmColorError 1
#define BxXpmSuccess 0
#define BxXpmOpenFailed -1
#define BxXpmFileInvalid -2
#define BxXpmNoMemory -3
#define BxXpmColorFailed -4

typedef struct {
	char *name;  /* Symbolic color name */
	char *value; /* Color value */
	Pixel pixel; /* Color pixel */
} BxXpmColorSymbol;

typedef struct {
	unsigned long valuemask; /* Specifies which attributes are
	                          * defined */

	Visual *visual;                 /* Specifies the visual to use */
	Colormap colormap;              /* Specifies the colormap to use */
	unsigned int depth;             /* Specifies the depth */
	unsigned int width;             /* Returns the width of the created
	                                 * pixmap */
	unsigned int height;            /* Returns the height of the created
	                                 * pixmap */
	unsigned int x_hotspot;         /* Returns the x hotspot's
	                                 * coordinate */
	unsigned int y_hotspot;         /* Returns the y hotspot's
	                                 * coordinate */
	unsigned int cpp;               /* Specifies the number of char per
	                                 * pixel */
	Pixel *pixels;                  /* List of used color pixels */
	unsigned int npixels;           /* Number of pixels */
	BxXpmColorSymbol *colorsymbols; /* Array of color symbols to
	                                 * override */
	unsigned int numsymbols;        /* Number of symbols */
	char *rgb_fname;                /* RGB text file name */

	/* Infos */
	unsigned int ncolors;    /* Number of colors */
	char ***colorTable;      /* Color table pointer */
	char *hints_cmt;         /* Comment of the hints section */
	char *colors_cmt;        /* Comment of the colors section */
	char *pixels_cmt;        /* Comment of the pixels section */
	unsigned int mask_pixel; /* Transparent pixel's color table
	                          * index */
} BxXpmAttributes;

/* Xpm attribute value masks bits */
#define BxXpmVisual (1L << 0)
#define BxXpmColormap (1L << 1)
#define BxXpmDepth (1L << 2)
#define BxXpmSize (1L << 3)    /* width & height */
#define BxXpmHotspot (1L << 4) /* x_hotspot & y_hotspot */
#define BxXpmCharsPerPixel (1L << 5)
#define BxXpmColorSymbols (1L << 6)
#define BxXpmRgbFilename (1L << 7)
#define BxXpmInfos (1L << 8) /* all infos members */

#define BxXpmReturnPixels (1L << 9)
#define BxXpmReturnInfos BxXpmInfos

/*
 * minimal portability layer between ansi and KR C
 */

/* forward declaration of functions with prototypes */

#ifdef NeedFunctionPrototypes
#define LFUNC(f, t, p) static t f p
#else
#define LFUNC(f, t, p) static t f()
#endif

/*
 * functions declarations
 */
LFUNC(BxXpmCreatePixmapFromData, int,
      (Display * display, Drawable d, char **data, Pixmap *pixmap_return, Pixmap *shapemask_return, BxXpmAttributes *attributes));

LFUNC(BxXpmCreateImageFromData, int,
      (Display * display, char **data, XImage **image_return, XImage **shapemask_return, BxXpmAttributes *attributes));

LFUNC(BxXpmFreeAttributes, void, (BxXpmAttributes * attributes));

typedef struct {
	unsigned int type;
	union {
		FILE *file;
		char **data;
	} stream;
	char *cptr;
	unsigned int line;
	int CommentLength;
	char Comment[BUFSIZ];
	char *Bcmt, *Ecmt, Bos, Eos;
	unsigned int InsideString; /* used during parsing: 0 or 1
	                            * whether we are inside or not */
} bxxpmData;

#define BXXPMARRAY 0
#define BXXPMFILE 1
#define BXXPMPIPE 2

typedef unsigned char byte;

#define BX_TRANSPARENT_COLOR "None" /* this must be a string! */

/* number of BxXpmColorKeys */
#define BXNKEYS 5

/*
 * key numbers for visual type, they must fit along with the number key of
 * each corresponding element in BxXpmColorKeys[] defined in xpm.h
 */
#define BXMONO 2
#define BXGRAY4 3
#define BXGRAY 4
#define BXCOLOR 5

/* structure containing data related to an Xpm pixmap */
typedef struct {
	char *name;
	unsigned int width;
	unsigned int height;
	unsigned int cpp;
	unsigned int ncolors;
	char ***colorTable;
	unsigned int *pixelindex;
	XColor *xcolors;
	char **colorStrings;
	unsigned int mask_pixel; /* mask pixel's colorTable index */
} bxxpmInternAttrib;

#define BX_UNDEF_PIXEL 0x80000000

char *BxXpmColorKeys[] = {
    "s",  /* key #1: symbol */
    "m",  /* key #2: mono visual */
    "g4", /* key #3: 4 grays visual */
    "g",  /* key #4: gray visual */
    "c",  /* key #5: color visual */
};

/* XPM private routines */

LFUNC(xpmCreateImage, int,
      (Display * display, bxxpmInternAttrib *attrib, XImage **image_return, XImage **shapeimage_return,
       BxXpmAttributes *attributes));

LFUNC(xpmParseData, int, (bxxpmData * data, bxxpmInternAttrib *attrib_return, BxXpmAttributes *attributes));

LFUNC(BxXpmVisualType, int, (Visual * visual));
LFUNC(xpmFreeColorTable, void, (char ***colorTable, int ncolors));

LFUNC(xpmInitInternAttrib, void, (bxxpmInternAttrib * xmpdata));

LFUNC(xpmFreeInternAttrib, void, (bxxpmInternAttrib * xmpdata));

LFUNC(xpmSetAttributes, void, (bxxpmInternAttrib * attrib, BxXpmAttributes *attributes));

/* I/O utility */

LFUNC(xpmNextString, void, (bxxpmData * mdata));
LFUNC(xpmNextUI, int, (bxxpmData * mdata, unsigned int *ui_return));
LFUNC(xpmGetC, int, (bxxpmData * mdata));
LFUNC(xpmUngetC, int, (int c, bxxpmData *mdata));
LFUNC(xpmNextWord, unsigned int, (bxxpmData * mdata, char *buf));
LFUNC(xpmGetCmt, void, (bxxpmData * mdata, char **cmt));
LFUNC(xpmOpenArray, int, (char **data, bxxpmData *mdata));
LFUNC(XpmDataClose, void, (bxxpmData * mdata));

/* RGB utility */

LFUNC(xpm_xynormalizeimagebits, void, (unsigned char *bp, XImage *img));
LFUNC(xpm_znormalizeimagebits, void, (unsigned char *bp, XImage *img));

/* Image utility */

LFUNC(SetColor, int,
      (Display * display, Colormap colormap, char *colorname, unsigned int color_index, Pixel *image_pixel, Pixel *mask_pixel,
       unsigned int *mask_pixel_index));

LFUNC(CreateXImage, int,
      (Display * display, Visual *visual, unsigned int depth, unsigned int width, unsigned int height, XImage **image_return));

LFUNC(SetImagePixels, void, (XImage * image, unsigned int width, unsigned int height, unsigned int *pixelindex, Pixel *pixels));

LFUNC(SetImagePixels32, void, (XImage * image, unsigned int width, unsigned int height, unsigned int *pixelindex, Pixel *pixels));

LFUNC(SetImagePixels16, void, (XImage * image, unsigned int width, unsigned int height, unsigned int *pixelindex, Pixel *pixels));

LFUNC(SetImagePixels8, void, (XImage * image, unsigned int width, unsigned int height, unsigned int *pixelindex, Pixel *pixels));

LFUNC(SetImagePixels1, void, (XImage * image, unsigned int width, unsigned int height, unsigned int *pixelindex, Pixel *pixels));

LFUNC(atoui, unsigned int, (char *p, unsigned int l, unsigned int *ui_return));

/*
 * Macros
 *
 * The BXXYNORMALIZE macro determines whether XY format data requires
 * normalization and calls a routine to do so if needed. The logic in
 * this module is designed for LSBFirst byte and bit order, so
 * normalization is done as required to present the data in this order.
 *
 * The BXZNORMALIZE macro performs byte and nibble order normalization if
 * required for Z format data.
 *
 * The BXXYINDEX macro computes the index to the starting byte (char) boundary
 * for a bitmap_unit containing a pixel with coordinates x and y for image
 * data in XY format.
 *
 * The BXZINDEX* macros compute the index to the starting byte (char) boundary
 * for a pixel with coordinates x and y for image data in ZPixmap format.
 *
 */

#define BXXYNORMALIZE(bp, img)                                                                                                   \
	if ((img->byte_order == MSBFirst) || (img->bitmap_bit_order == MSBFirst))                                                    \
	xpm_xynormalizeimagebits((unsigned char *)(bp), img)

#define BXZNORMALIZE(bp, img)                                                                                                    \
	if (img->byte_order == MSBFirst)                                                                                             \
	xpm_znormalizeimagebits((unsigned char *)(bp), img)

#define BXXYINDEX(x, y, img) ((y)*img->bytes_per_line) + (((x) + img->xoffset) / img->bitmap_unit) * (img->bitmap_unit >> 3)

#define BXZINDEX(x, y, img) ((y)*img->bytes_per_line) + (((x)*img->bits_per_pixel) >> 3)

#define BXZINDEX32(x, y, img) ((y)*img->bytes_per_line) + ((x) << 2)

#define BXZINDEX16(x, y, img) ((y)*img->bytes_per_line) + ((x) << 1)

#define BXZINDEX8(x, y, img) ((y)*img->bytes_per_line) + (x)

#define BXZINDEX1(x, y, img) ((y)*img->bytes_per_line) + ((x) >> 3)

#if __STDC__
#define Const const
#else
#define Const
#endif

static unsigned int atoui(char *p, unsigned int l, unsigned int *ui_return) {
	int n, i;

	n = 0;
	for (i = 0; i < l; i++)
		if (*p >= '0' && *p <= '9')
			n = n * 10 + *p++ - '0';
		else
			break;

	if (i != 0 && i == l) {
		*ui_return = n;
		return 1;
	}
	else
		return 0;
}

static int BxXpmCreatePixmapFromData(
    Display *display, Drawable d, char **data, Pixmap *pixmap_return,
    Pixmap *shapemask_return, BxXpmAttributes *attributes) {
	XImage *image, **imageptr = NULL;
	XImage *shapeimage, **shapeimageptr = NULL;
	int ErrorStatus;
	XGCValues gcv;
	GC gc;

	/*
	 * initialize return values
	 */
	if (pixmap_return) {
		*pixmap_return = (Pixmap)NULL;
		imageptr = &image;
	}
	if (shapemask_return) {
		*shapemask_return = (Pixmap)NULL;
		shapeimageptr = &shapeimage;
	}

	/*
	 * create the images
	 */
	ErrorStatus = BxXpmCreateImageFromData(display, data, imageptr, shapeimageptr, attributes);
	if (ErrorStatus < 0)
		return (ErrorStatus);

	/*
	 * create the pixmaps
	 */
	if (imageptr && image) {
		*pixmap_return = XCreatePixmap(display, d, image->width, image->height, image->depth);
		gcv.function = GXcopy;
		gc = XCreateGC(display, *pixmap_return, GCFunction, &gcv);

		XPutImage(display, *pixmap_return, gc, image, 0, 0, 0, 0, image->width, image->height);

		XDestroyImage(image);
		XFreeGC(display, gc);
	}
	if (shapeimageptr && shapeimage) {
		*shapemask_return = XCreatePixmap(display, d, shapeimage->width, shapeimage->height, shapeimage->depth);
		gcv.function = GXcopy;
		gc = XCreateGC(display, *shapemask_return, GCFunction, &gcv);

		XPutImage(display, *shapemask_return, gc, shapeimage, 0, 0, 0, 0, shapeimage->width, shapeimage->height);

		XDestroyImage(shapeimage);
		XFreeGC(display, gc);
	}
	return (ErrorStatus);
}

static int BxXpmCreateImageFromData(
    Display *display, char **data, XImage **image_return,
    XImage **shapeimage_return, BxXpmAttributes *attributes) {
	bxxpmData mdata;
	int ErrorStatus;
	bxxpmInternAttrib attrib;

	/*
	 * initialize return values
	 */
	if (image_return)
		*image_return = NULL;
	if (shapeimage_return)
		*shapeimage_return = NULL;

	if ((ErrorStatus = xpmOpenArray(data, &mdata)) != BxXpmSuccess)
		return (ErrorStatus);

	xpmInitInternAttrib(&attrib);

	ErrorStatus = xpmParseData(&mdata, &attrib, attributes);

	if (ErrorStatus == BxXpmSuccess)
		ErrorStatus = xpmCreateImage(display, &attrib, image_return, shapeimage_return, attributes);

	if (ErrorStatus >= 0)
		xpmSetAttributes(&attrib, attributes);
	else if (attributes)
		BxXpmFreeAttributes(attributes);

	xpmFreeInternAttrib(&attrib);
	XpmDataClose(&mdata);

	return (ErrorStatus);
}

/*
 * open the given array to be read or written as an bxxpmData which is returned
 */
static int xpmOpenArray(char **data, bxxpmData *mdata) {
	mdata->type = BXXPMARRAY;
	mdata->stream.data = data;
	mdata->cptr = *data;
	mdata->line = 0;
	mdata->CommentLength = 0;
	mdata->Bcmt = mdata->Ecmt = NULL;
	mdata->Bos = mdata->Eos = '\0';
	mdata->InsideString = 0;
	return (BxXpmSuccess);
}

/*
 * Intialize the bxxpmInternAttrib pointers to Null to know
 * which ones must be freed later on.
 */
static void xpmInitInternAttrib(bxxpmInternAttrib *attrib) {
	attrib->ncolors = 0;
	attrib->colorTable = NULL;
	attrib->pixelindex = NULL;
	attrib->xcolors = NULL;
	attrib->colorStrings = NULL;
	attrib->mask_pixel = BX_UNDEF_PIXEL;
}

/* function call in case of error, frees only localy allocated variables */
#undef RETURN
#define RETURN(status)                                           \
	{                                                        \
		if (colorTable)                                  \
			xpmFreeColorTable(colorTable, ncolors);  \
		if (chars)                                       \
			free(chars);                             \
		if (pixelindex)                                  \
			free((char *)pixelindex);                \
		if (hints_cmt)                                   \
			free((char *)hints_cmt);                 \
		if (colors_cmt)                                  \
			free((char *)colors_cmt);                \
		if (pixels_cmt)                                  \
			free((char *)pixels_cmt);                \
		return (status);                                 \
	}

/*
 * This function parses an Xpm file or data and store the found informations
 * in an an bxxpmInternAttrib structure which is returned.
 */
static int xpmParseData(
    bxxpmData *data, bxxpmInternAttrib *attrib_return,
    BxXpmAttributes *attributes) {
	/* variables to return */
	unsigned int width, height;
	unsigned int ncolors = 0;
	unsigned int cpp;
	unsigned int x_hotspot, y_hotspot, hotspot = 0;
	char ***colorTable = NULL;
	unsigned int *pixelindex = NULL;
	char *hints_cmt = NULL;
	char *colors_cmt = NULL;
	char *pixels_cmt = NULL;

	/* calculation variables */
	unsigned int rncolors = 0; /* read number of colors, it is
	                            * different to ncolors to avoid
	                            * problem when freeing the
	                            * colorTable in case an error
	                            * occurs while reading the hints
	                            * line */
	unsigned int key;          /* color key */
	char *chars = NULL, buf[BUFSIZ];
	unsigned int *iptr;
	unsigned int a, b, x, y, l;

	unsigned int curkey;     /* current color key */
	unsigned int lastwaskey; /* key read */
	char curbuf[BUFSIZ];     /* current buffer */

	/*
	 * read hints: width, height, ncolors, chars_per_pixel
	 */
	if (!(xpmNextUI(data, &width) && xpmNextUI(data, &height) && xpmNextUI(data, &rncolors) && xpmNextUI(data, &cpp)))
		RETURN(BxXpmFileInvalid);

	ncolors = rncolors;

	/*
	 * read hotspot coordinates if any
	 */
	hotspot = xpmNextUI(data, &x_hotspot) && xpmNextUI(data, &y_hotspot);

	/*
	 * store the hints comment line
	 */
	if (attributes && (attributes->valuemask & BxXpmReturnInfos))
		xpmGetCmt(data, &hints_cmt);

	/*
	 * read colors
	 */
	colorTable = (char ***)calloc(ncolors, sizeof(char **));
	if (!colorTable)
		RETURN(BxXpmNoMemory);

	for (a = 0; a < ncolors; a++) {
		xpmNextString(data); /* skip the line */
		colorTable[a] = (char **)calloc((BXNKEYS + 1), sizeof(char *));
		if (!colorTable[a])
			RETURN(BxXpmNoMemory);

		/*
		 * read pixel value
		 */
		colorTable[a][0] = (char *)malloc(cpp);
		if (!colorTable[a][0])
			RETURN(BxXpmNoMemory);
		for (b = 0; b < cpp; b++)
			colorTable[a][0][b] = xpmGetC(data);

		/*
		 * read color keys and values
		 */
		curkey = 0;
		lastwaskey = 0;
		while ((l = xpmNextWord(data, buf))) {
			if (!lastwaskey) {
				for (key = 1; key < BXNKEYS + 1; key++)
					if ((strlen(BxXpmColorKeys[key - 1]) == l) && (!strncmp(BxXpmColorKeys[key - 1], buf, l)))
						break;
			}
			if (!lastwaskey && key <= BXNKEYS) { /* open new key */
				if (curkey) {                    /* flush string */
					colorTable[a][curkey] = (char *)malloc(strlen(curbuf) + 1);
					if (!colorTable[a][curkey])
						RETURN(BxXpmNoMemory);
					strcpy(colorTable[a][curkey], curbuf);
				}
				curkey = key;     /* set new key  */
				curbuf[0] = '\0'; /* reset curbuf */
				lastwaskey = 1;
			}
			else {
				if (!curkey)
					RETURN(BxXpmFileInvalid); /* key without value */
				if (!lastwaskey)
					strcat(curbuf, " "); /* append space */
				buf[l] = '\0';
				strcat(curbuf, buf); /* append buf */
				lastwaskey = 0;
			}
		}
		if (!curkey)
			RETURN(BxXpmFileInvalid); /* key without value */
		colorTable[a][curkey] = (char *)malloc(strlen(curbuf) + 1);
		if (!colorTable[a][curkey])
			RETURN(BxXpmNoMemory);
		strcpy(colorTable[a][curkey], curbuf);
	}

	/*
	 * store the colors comment line
	 */
	if (attributes && (attributes->valuemask & BxXpmReturnInfos))
		xpmGetCmt(data, &colors_cmt);

	/*
	 * read pixels and index them on color number
	 */
	pixelindex = (unsigned int *)malloc(sizeof(unsigned int) * width * height);
	if (!pixelindex)
		RETURN(BxXpmNoMemory);

	iptr = pixelindex;

	chars = (char *)malloc(cpp);
	if (!chars)
		RETURN(BxXpmNoMemory);

	for (y = 0; y < height; y++) {
		xpmNextString(data);
		for (x = 0; x < width; x++, iptr++) {
			for (a = 0; a < cpp; a++)
				chars[a] = xpmGetC(data);
			for (a = 0; a < ncolors; a++)
				if (!strncmp(colorTable[a][0], chars, cpp))
					break;
			if (a == ncolors)
				RETURN(BxXpmFileInvalid); /* no color matches */
			*iptr = a;
		}
	}

	/*
	 * store the pixels comment line
	 */
	if (attributes && (attributes->valuemask & BxXpmReturnInfos))
		xpmGetCmt(data, &pixels_cmt);

	free(chars);

	/*
	 * store found informations in the bxxpmInternAttrib structure
	 */
	attrib_return->width = width;
	attrib_return->height = height;
	attrib_return->cpp = cpp;
	attrib_return->ncolors = ncolors;
	attrib_return->colorTable = colorTable;
	attrib_return->pixelindex = pixelindex;

	if (attributes) {
		if (attributes->valuemask & BxXpmReturnInfos) {
			attributes->hints_cmt = hints_cmt;
			attributes->colors_cmt = colors_cmt;
			attributes->pixels_cmt = pixels_cmt;
		}
		if (hotspot) {
			attributes->x_hotspot = x_hotspot;
			attributes->y_hotspot = y_hotspot;
			attributes->valuemask |= BxXpmHotspot;
		}
	}
	return (BxXpmSuccess);
}

/*
 * set the color pixel related to the given colorname,
 * return 0 if success, 1 otherwise.
 */

static int SetColor(
    Display *display, Colormap colormap, char *colorname, unsigned int color_index,
    Pixel *image_pixel, Pixel *mask_pixel, unsigned int *mask_pixel_index) {
	XColor xcolor;

	if (strcasecmp(colorname, BX_TRANSPARENT_COLOR)) {
		if (!XParseColor(display, colormap, colorname, &xcolor) || (!XAllocColor(display, colormap, &xcolor)))
			return (1);
		*image_pixel = xcolor.pixel;
		*mask_pixel = 1;
	}
	else {
		*image_pixel = 0;
		*mask_pixel = 0;
		*mask_pixel_index = color_index; /* store the color table index */
	}
	return (0);
}

/* function call in case of error, frees only localy allocated variables */
#undef RETURN
#define RETURN(status)                               \
	{                                            \
		if (image)                           \
			XDestroyImage(image);        \
		if (shapeimage)                      \
			XDestroyImage(shapeimage);   \
		if (image_pixels)                    \
			free((char *)image_pixels);  \
		if (mask_pixels)                     \
			free((char *)mask_pixels);   \
		return (status);                     \
	}

static int xpmCreateImage(
    Display *display, bxxpmInternAttrib *attrib, XImage **image_return,
    XImage **shapeimage_return, BxXpmAttributes *attributes) {
	/* variables stored in the BxXpmAttributes structure */
	Visual *visual;
	Colormap colormap;
	unsigned int depth;
	BxXpmColorSymbol *colorsymbols;
	unsigned int numsymbols;

	/* variables to return */
	XImage *image = NULL;
	XImage *shapeimage = NULL;
	unsigned int mask_pixel;
	unsigned int ErrorStatus, ErrorStatus2;

	/* calculation variables */
	Pixel *image_pixels = NULL;
	Pixel *mask_pixels = NULL;
	char *colorname;
	unsigned int a, b, l;
	Boolean pixel_defined;
	unsigned int key;

	/*
	 * retrieve information from the BxXpmAttributes
	 */
	if (attributes && attributes->valuemask & BxXpmColorSymbols) {
		colorsymbols = attributes->colorsymbols;
		numsymbols = attributes->numsymbols;
	}
	else
		numsymbols = 0;

	if (attributes && attributes->valuemask & BxXpmVisual)
		visual = attributes->visual;
	else
		visual = DefaultVisual(display, DefaultScreen(display));

	if (attributes && attributes->valuemask & BxXpmColormap)
		colormap = attributes->colormap;
	else
		colormap = DefaultColormap(display, DefaultScreen(display));

	if (attributes && attributes->valuemask & BxXpmDepth)
		depth = attributes->depth;
	else
		depth = DefaultDepth(display, DefaultScreen(display));

	ErrorStatus = BxXpmSuccess;

	/*
	 * alloc pixels index tables
	 */

	key = BxXpmVisualType(visual);
	image_pixels = (Pixel *)malloc(sizeof(Pixel) * attrib->ncolors);
	if (!image_pixels)
		RETURN(BxXpmNoMemory);

	mask_pixels = (Pixel *)malloc(sizeof(Pixel) * attrib->ncolors);
	if (!mask_pixels)
		RETURN(BxXpmNoMemory);

	mask_pixel = BX_UNDEF_PIXEL;

	/*
	 * get pixel colors, store them in index tables
	 */
	for (a = 0; a < attrib->ncolors; a++) {
		colorname = NULL;
		pixel_defined = False;

		/*
		 * look for a defined symbol
		 */
		if (numsymbols && attrib->colorTable[a][1]) {
			for (l = 0; l < numsymbols; l++)
				if (!strcmp(colorsymbols[l].name, attrib->colorTable[a][1]))
					break;
			if (l != numsymbols) {
				if (colorsymbols[l].value)
					colorname = colorsymbols[l].value;
				else
					pixel_defined = True;
			}
		}
		if (!pixel_defined) { /* pixel not given as symbol value */

			if (colorname) { /* colorname given as symbol value */
				if (!SetColor(display, colormap, colorname, a, &image_pixels[a], &mask_pixels[a], &mask_pixel))
					pixel_defined = True;
				else
					ErrorStatus = BxXpmColorError;
			}
			b = key;
			while (!pixel_defined && b > 1) {
				if (attrib->colorTable[a][b]) {
					if (!SetColor(display, colormap, attrib->colorTable[a][b], a, &image_pixels[a], &mask_pixels[a],
					              &mask_pixel)) {
						pixel_defined = True;
						break;
					}
					else
						ErrorStatus = BxXpmColorError;
				}
				b--;
			}

			b = key + 1;
			while (!pixel_defined && b < BXNKEYS + 1) {
				if (attrib->colorTable[a][b]) {
					if (!SetColor(display, colormap, attrib->colorTable[a][b], a, &image_pixels[a], &mask_pixels[a],
					              &mask_pixel)) {
						pixel_defined = True;
						break;
					}
					else
						ErrorStatus = BxXpmColorError;
				}
				b++;
			}

			if (!pixel_defined)
				RETURN(BxXpmColorFailed);
		}
		else {
			image_pixels[a] = colorsymbols[l].pixel;
			mask_pixels[a] = 1;
		}
	}

	/*
	 * create the image
	 */
	if (image_return) {
		ErrorStatus2 = CreateXImage(display, visual, depth, attrib->width, attrib->height, &image);
		if (ErrorStatus2 != BxXpmSuccess)
			RETURN(ErrorStatus2);

		/*
		 * set the image data
		 *
		 * In case depth is 1 or bits_per_pixel is 4, 6, 8, 24 or 32 use
		 * optimized functions, otherwise use slower but sure general one.
		 *
		 */

		if (image->depth == 1)
			SetImagePixels1(image, attrib->width, attrib->height, attrib->pixelindex, image_pixels);
		else if (image->bits_per_pixel == 8)
			SetImagePixels8(image, attrib->width, attrib->height, attrib->pixelindex, image_pixels);
		else if (image->bits_per_pixel == 16)
			SetImagePixels16(image, attrib->width, attrib->height, attrib->pixelindex, image_pixels);
		else if (image->bits_per_pixel == 32)
			SetImagePixels32(image, attrib->width, attrib->height, attrib->pixelindex, image_pixels);
		else
			SetImagePixels(image, attrib->width, attrib->height, attrib->pixelindex, image_pixels);
	}

	/*
	 * create the shape mask image
	 */
	if (mask_pixel != BX_UNDEF_PIXEL && shapeimage_return) {
		ErrorStatus2 = CreateXImage(display, visual, 1, attrib->width, attrib->height, &shapeimage);
		if (ErrorStatus2 != BxXpmSuccess)
			RETURN(ErrorStatus2);

		SetImagePixels1(shapeimage, attrib->width, attrib->height, attrib->pixelindex, mask_pixels);
	}
	free((char *)mask_pixels);

	/*
	 * if requested store allocated pixels in the BxXpmAttributes structure
	 */
	if (attributes && (attributes->valuemask & BxXpmReturnInfos || attributes->valuemask & BxXpmReturnPixels)) {
		if (mask_pixel != BX_UNDEF_PIXEL) {
			Pixel *pixels, *p1, *p2;

			attributes->npixels = attrib->ncolors - 1;
			pixels = (Pixel *)malloc(sizeof(Pixel) * attributes->npixels);
			if (pixels) {
				p1 = image_pixels;
				p2 = pixels;
				for (a = 0; a < attrib->ncolors; a++, p1++)
					if (a != mask_pixel)
						*p2++ = *p1;
				attributes->pixels = pixels;
			}
			else {
				/* if error just say we can't return requested data */
				attributes->valuemask &= ~BxXpmReturnPixels;
				attributes->valuemask &= ~BxXpmReturnInfos;
				attributes->pixels = NULL;
				attributes->npixels = 0;
			}
			free((char *)image_pixels);
		}
		else {
			attributes->pixels = image_pixels;
			attributes->npixels = attrib->ncolors;
		}
		attributes->mask_pixel = mask_pixel;
	}
	else
		free((char *)image_pixels);

	/*
	 * return created images
	 */
	if (image_return)
		*image_return = image;

	if (shapeimage_return)
		*shapeimage_return = shapeimage;

	return (ErrorStatus);
}

/*
 * Create an XImage
 */
static int CreateXImage(
    Display *display, Visual *visual, unsigned int depth, unsigned int width,
    unsigned int height, XImage **image_return) {
	int bitmap_pad;

	/* first get bitmap_pad */
	if (depth > 16)
		bitmap_pad = 32;
	else if (depth > 8)
		bitmap_pad = 16;
	else
		bitmap_pad = 8;

	/* then create the XImage with data = NULL and bytes_per_line = 0 */

	*image_return = XCreateImage(display, visual, depth, ZPixmap, 0, 0, width, height, bitmap_pad, 0);
	if (!*image_return)
		return (BxXpmNoMemory);

	/* now that bytes_per_line must have been set properly alloc data */

	(*image_return)->data = (char *)malloc((*image_return)->bytes_per_line * height);

	if (!(*image_return)->data) {
		XDestroyImage(*image_return);
		*image_return = NULL;
		return (BxXpmNoMemory);
	}
	return (BxXpmSuccess);
}

/*
 * The functions below are written from X11R5 MIT's code (XImUtil.c)
 *
 * The idea is to have faster functions than the standard XPutPixel function
 * to build the image data. Indeed we can speed up things by suppressing tests
 * performed for each pixel. We do exactly the same tests but at the image
 * level. Assuming that we use only ZPixmap images.
 */

LFUNC(_putbits, void, (char *src, int dstoffset, int numbits, char *dst));

LFUNC(_XReverse_Bytes, void, (unsigned char *bpt, int nb));

static unsigned char Const _reverse_byte[0x100] = {
    0x00, 0x80, 0x40, 0xc0, 0x20, 0xa0, 0x60, 0xe0, 0x10, 0x90, 0x50, 0xd0, 0x30, 0xb0, 0x70, 0xf0, 0x08, 0x88, 0x48, 0xc8,
    0x28, 0xa8, 0x68, 0xe8, 0x18, 0x98, 0x58, 0xd8, 0x38, 0xb8, 0x78, 0xf8, 0x04, 0x84, 0x44, 0xc4, 0x24, 0xa4, 0x64, 0xe4,
    0x14, 0x94, 0x54, 0xd4, 0x34, 0xb4, 0x74, 0xf4, 0x0c, 0x8c, 0x4c, 0xcc, 0x2c, 0xac, 0x6c, 0xec, 0x1c, 0x9c, 0x5c, 0xdc,
    0x3c, 0xbc, 0x7c, 0xfc, 0x02, 0x82, 0x42, 0xc2, 0x22, 0xa2, 0x62, 0xe2, 0x12, 0x92, 0x52, 0xd2, 0x32, 0xb2, 0x72, 0xf2,
    0x0a, 0x8a, 0x4a, 0xca, 0x2a, 0xaa, 0x6a, 0xea, 0x1a, 0x9a, 0x5a, 0xda, 0x3a, 0xba, 0x7a, 0xfa, 0x06, 0x86, 0x46, 0xc6,
    0x26, 0xa6, 0x66, 0xe6, 0x16, 0x96, 0x56, 0xd6, 0x36, 0xb6, 0x76, 0xf6, 0x0e, 0x8e, 0x4e, 0xce, 0x2e, 0xae, 0x6e, 0xee,
    0x1e, 0x9e, 0x5e, 0xde, 0x3e, 0xbe, 0x7e, 0xfe, 0x01, 0x81, 0x41, 0xc1, 0x21, 0xa1, 0x61, 0xe1, 0x11, 0x91, 0x51, 0xd1,
    0x31, 0xb1, 0x71, 0xf1, 0x09, 0x89, 0x49, 0xc9, 0x29, 0xa9, 0x69, 0xe9, 0x19, 0x99, 0x59, 0xd9, 0x39, 0xb9, 0x79, 0xf9,
    0x05, 0x85, 0x45, 0xc5, 0x25, 0xa5, 0x65, 0xe5, 0x15, 0x95, 0x55, 0xd5, 0x35, 0xb5, 0x75, 0xf5, 0x0d, 0x8d, 0x4d, 0xcd,
    0x2d, 0xad, 0x6d, 0xed, 0x1d, 0x9d, 0x5d, 0xdd, 0x3d, 0xbd, 0x7d, 0xfd, 0x03, 0x83, 0x43, 0xc3, 0x23, 0xa3, 0x63, 0xe3,
    0x13, 0x93, 0x53, 0xd3, 0x33, 0xb3, 0x73, 0xf3, 0x0b, 0x8b, 0x4b, 0xcb, 0x2b, 0xab, 0x6b, 0xeb, 0x1b, 0x9b, 0x5b, 0xdb,
    0x3b, 0xbb, 0x7b, 0xfb, 0x07, 0x87, 0x47, 0xc7, 0x27, 0xa7, 0x67, 0xe7, 0x17, 0x97, 0x57, 0xd7, 0x37, 0xb7, 0x77, 0xf7,
    0x0f, 0x8f, 0x4f, 0xcf, 0x2f, 0xaf, 0x6f, 0xef, 0x1f, 0x9f, 0x5f, 0xdf, 0x3f, 0xbf, 0x7f, 0xff};

static void _XReverse_Bytes(unsigned char *bpt, int nb) {
	do {
		*bpt = _reverse_byte[*bpt];
		bpt++;
	} while (--nb > 0);
}

static void xpm_xynormalizeimagebits(unsigned char *bp, XImage *img) {
	unsigned char c;

	if (img->byte_order != img->bitmap_bit_order) {
		switch (img->bitmap_unit) {

		case 16:
			c = *bp;
			*bp = *(bp + 1);
			*(bp + 1) = c;
			break;

		case 32:
			c = *(bp + 3);
			*(bp + 3) = *bp;
			*bp = c;
			c = *(bp + 2);
			*(bp + 2) = *(bp + 1);
			*(bp + 1) = c;
			break;
		}
	}
	if (img->bitmap_bit_order == MSBFirst)
		_XReverse_Bytes(bp, img->bitmap_unit >> 3);
}

static void xpm_znormalizeimagebits(unsigned char *bp, XImage *img) {
	unsigned char c;

	switch (img->bits_per_pixel) {

	case 4:
		*bp = ((*bp >> 4) & 0xF) | ((*bp << 4) & ~0xF);
		break;

	case 16:
		c = *bp;
		*bp = *(bp + 1);
		*(bp + 1) = c;
		break;

	case 24:
		c = *(bp + 2);
		*(bp + 2) = *bp;
		*bp = c;
		break;

	case 32:
		c = *(bp + 3);
		*(bp + 3) = *bp;
		*bp = c;
		c = *(bp + 2);
		*(bp + 2) = *(bp + 1);
		*(bp + 1) = c;
		break;
	}
}

static unsigned char Const _lomask[0x09] = {0x00, 0x01, 0x03, 0x07, 0x0f, 0x1f, 0x3f, 0x7f, 0xff};
static unsigned char Const _himask[0x09] = {0xff, 0xfe, 0xfc, 0xf8, 0xf0, 0xe0, 0xc0, 0x80, 0x00};

static void _putbits(
    char *src,      // address of source bit string
    int dstoffset,  // bit offset into destination; range is 0-31
    int numbits,    // number of bits to copy to destination
    char *dst)      // address of destination bit string
{
	unsigned char chlo, chhi;
	int hibits;

	dst = dst + (dstoffset >> 3);
	dstoffset = dstoffset & 7;
	hibits = 8 - dstoffset;
	chlo = *dst & _lomask[dstoffset];
	for (;;) {
		chhi = (*src << dstoffset) & _himask[dstoffset];
		if (numbits <= hibits) {
			chhi = chhi & _lomask[dstoffset + numbits];
			*dst = (*dst & _himask[dstoffset + numbits]) | chlo | chhi;
			break;
		}
		*dst = chhi | chlo;
		dst++;
		numbits = numbits - hibits;
		chlo = (unsigned char)(*src & _himask[hibits]) >> hibits;
		src++;
		if (numbits <= dstoffset) {
			chlo = chlo & _lomask[numbits];
			*dst = (*dst & _himask[numbits]) | chlo;
			break;
		}
		numbits = numbits - dstoffset;
	}
}

/*
 * Default method to write pixels into a Z image data structure.
 * The algorithm used is:
 *
 *	copy the destination bitmap_unit or Zpixel to temp
 *	normalize temp if needed
 *	copy the pixel bits into the temp
 *	renormalize temp if needed
 *	copy the temp back into the destination image data
 */

static void SetImagePixels(
    XImage *image, unsigned int width, unsigned int height,
    unsigned int *pixelindex, Pixel *pixels) {
	Pixel pixel;
	unsigned long px;
	char *src;
	char *dst;
	int nbytes;
	unsigned int *iptr;
	int x, y, i;

	iptr = pixelindex;
	if (image->depth == 1) {
		for (y = 0; y < height; y++)
			for (x = 0; x < width; x++, iptr++) {
				pixel = pixels[*iptr];
				for (i = 0, px = pixel; i < sizeof(unsigned long); i++, px >>= 8)
					((unsigned char *)&pixel)[i] = (unsigned char)px;
				src = &image->data[BXXYINDEX(x, y, image)];
				dst = (char *)&px;
				px = 0;
				nbytes = image->bitmap_unit >> 3;
				for (i = nbytes; --i >= 0;)
					*dst++ = *src++;
				BXXYNORMALIZE(&px, image);
				i = ((x + image->xoffset) % image->bitmap_unit);
				_putbits((char *)&pixel, i, 1, (char *)&px);
				BXXYNORMALIZE(&px, image);
				src = (char *)&px;
				dst = &image->data[BXXYINDEX(x, y, image)];
				for (i = nbytes; --i >= 0;)
					*dst++ = *src++;
			}
	}
	else {
		for (y = 0; y < height; y++)
			for (x = 0; x < width; x++, iptr++) {
				pixel = pixels[*iptr];
				if (image->depth == 4)
					pixel &= 0xf;
				for (i = 0, px = pixel; i < sizeof(unsigned long); i++, px >>= 8)
					((unsigned char *)&pixel)[i] = (unsigned char)px;
				src = &image->data[BXZINDEX(x, y, image)];
				dst = (char *)&px;
				px = 0;
				nbytes = (image->bits_per_pixel + 7) >> 3;
				for (i = nbytes; --i >= 0;)
					*dst++ = *src++;
				BXZNORMALIZE(&px, image);
				_putbits((char *)&pixel, (x * image->bits_per_pixel) & 7, image->bits_per_pixel, (char *)&px);
				BXZNORMALIZE(&px, image);
				src = (char *)&px;
				dst = &image->data[BXZINDEX(x, y, image)];
				for (i = nbytes; --i >= 0;)
					*dst++ = *src++;
			}
	}
}

/*
 * write pixels into a 32-bits Z image data structure
 */

#ifndef WORD64
static unsigned long byteorderpixel = MSBFirst << 24;

#endif

static void SetImagePixels32(
    XImage *image, unsigned int width, unsigned int height,
    unsigned int *pixelindex, Pixel *pixels) {
	unsigned char *addr;
	unsigned int *paddr;
	unsigned int *iptr;
	int x, y;

	iptr = pixelindex;
#ifndef WORD64
	if (*((char *)&byteorderpixel) == image->byte_order) {
		for (y = 0; y < height; y++)
			for (x = 0; x < width; x++, iptr++) {
				paddr = (unsigned int *)(&(image->data[BXZINDEX32(x, y, image)]));
				*paddr = (unsigned int)pixels[*iptr];
			}
	}
	else
#endif
	    if (image->byte_order == MSBFirst)
		for (y = 0; y < height; y++)
			for (x = 0; x < width; x++, iptr++) {
				addr = &((unsigned char *)image->data)[BXZINDEX32(x, y, image)];
				addr[0] = (unsigned char)(pixels[*iptr] >> 24);
				addr[1] = (unsigned char)(pixels[*iptr] >> 16);
				addr[2] = (unsigned char)(pixels[*iptr] >> 8);
				addr[3] = (unsigned char)(pixels[*iptr]);
			}
	else
		for (y = 0; y < height; y++)
			for (x = 0; x < width; x++, iptr++) {
				addr = &((unsigned char *)image->data)[BXZINDEX32(x, y, image)];
				addr[3] = (unsigned char)(pixels[*iptr] >> 24);
				addr[2] = (unsigned char)(pixels[*iptr] >> 16);
				addr[1] = (unsigned char)(pixels[*iptr] >> 8);
				addr[0] = (unsigned char)(pixels[*iptr]);
			}
}

/*
 * write pixels into a 16-bits Z image data structure
 */

static void SetImagePixels16(
    XImage *image, unsigned int width, unsigned int height,
    unsigned int *pixelindex, Pixel *pixels) {
	unsigned char *addr;
	unsigned int *iptr;
	int x, y;

	iptr = pixelindex;
	if (image->byte_order == MSBFirst)
		for (y = 0; y < height; y++)
			for (x = 0; x < width; x++, iptr++) {
				addr = &((unsigned char *)image->data)[BXZINDEX16(x, y, image)];
				addr[0] = (unsigned char)(pixels[*iptr] >> 8);
				addr[1] = (unsigned char)(pixels[*iptr]);
			}
	else
		for (y = 0; y < height; y++)
			for (x = 0; x < width; x++, iptr++) {
				addr = &((unsigned char *)image->data)[BXZINDEX16(x, y, image)];
				addr[1] = (unsigned char)(pixels[*iptr] >> 8);
				addr[0] = (unsigned char)(pixels[*iptr]);
			}
}

/*
 * write pixels into a 8-bits Z image data structure
 */

static void SetImagePixels8(
    XImage *image, unsigned int width, unsigned int height,
    unsigned int *pixelindex, Pixel *pixels) {
	unsigned int *iptr;
	int x, y;

	iptr = pixelindex;
	for (y = 0; y < height; y++)
		for (x = 0; x < width; x++, iptr++)
			image->data[BXZINDEX8(x, y, image)] = (char)pixels[*iptr];
}

/*
 * write pixels into a 1-bit depth image data structure and **offset null**
 */

static void SetImagePixels1(
    XImage *image, unsigned int width, unsigned int height,
    unsigned int *pixelindex, Pixel *pixels) {
	unsigned char bit;
	int xoff, yoff;
	unsigned int *iptr;
	int x, y;

	if (image->byte_order != image->bitmap_bit_order)
		SetImagePixels(image, width, height, pixelindex, pixels);
	else {
		iptr = pixelindex;
		if (image->bitmap_bit_order == MSBFirst)
			for (y = 0; y < height; y++)
				for (x = 0; x < width; x++, iptr++) {
					yoff = BXZINDEX1(x, y, image);
					xoff = x & 7;
					bit = 0x80 >> xoff;
					if (pixels[*iptr] & 1)
						image->data[yoff] |= bit;
					else
						image->data[yoff] &= ~bit;
				}
		else
			for (y = 0; y < height; y++)
				for (x = 0; x < width; x++, iptr++) {
					yoff = BXZINDEX1(x, y, image);
					xoff = x & 7;
					bit = 1 << xoff;
					if (pixels[*iptr] & 1)
						image->data[yoff] |= bit;
					else
						image->data[yoff] &= ~bit;
				}
	}
}

/*
 * Store into the BxXpmAttributes structure the required informations stored in
 * the bxxpmInternAttrib structure.
 */

static void xpmSetAttributes(
    bxxpmInternAttrib *attrib, BxXpmAttributes *attributes) {
	if (attributes) {
		if (attributes->valuemask & BxXpmReturnInfos) {
			attributes->cpp = attrib->cpp;
			attributes->ncolors = attrib->ncolors;
			attributes->colorTable = attrib->colorTable;

			attrib->ncolors = 0;
			attrib->colorTable = NULL;
		}
		attributes->width = attrib->width;
		attributes->height = attrib->height;
		attributes->valuemask |= BxXpmSize;
	}
}

/*
 * Free the BxXpmAttributes structure members
 * but the structure itself
 */

static void BxXpmFreeAttributes(BxXpmAttributes *attributes) {
	if (attributes) {
		if (attributes->valuemask & BxXpmReturnPixels && attributes->pixels) {
			free((char *)attributes->pixels);
			attributes->pixels = NULL;
			attributes->npixels = 0;
		}
		if (attributes->valuemask & BxXpmInfos) {
			if (attributes->colorTable) {
				xpmFreeColorTable(attributes->colorTable, attributes->ncolors);
				attributes->colorTable = NULL;
				attributes->ncolors = 0;
			}
			if (attributes->hints_cmt) {
				free(attributes->hints_cmt);
				attributes->hints_cmt = NULL;
			}
			if (attributes->colors_cmt) {
				free(attributes->colors_cmt);
				attributes->colors_cmt = NULL;
			}
			if (attributes->pixels_cmt) {
				free(attributes->pixels_cmt);
				attributes->pixels_cmt = NULL;
			}
			if (attributes->pixels) {
				free((char *)attributes->pixels);
				attributes->pixels = NULL;
			}
		}
		attributes->valuemask = 0;
	}
}

/*
 * Free the bxxpmInternAttrib pointers which have been allocated
 */

static void xpmFreeInternAttrib(bxxpmInternAttrib *attrib) {
	unsigned int a;

	if (attrib->colorTable)
		xpmFreeColorTable(attrib->colorTable, attrib->ncolors);
	if (attrib->pixelindex)
		free((char *)attrib->pixelindex);
	if (attrib->xcolors)
		free((char *)attrib->xcolors);
	if (attrib->colorStrings) {
		for (a = 0; a < attrib->ncolors; a++)
			if (attrib->colorStrings[a])
				free((char *)attrib->colorStrings[a]);
		free((char *)attrib->colorStrings);
	}
}

/*
 * close the file related to the bxxpmData if any
 */
static void XpmDataClose(bxxpmData *mdata) {
	switch (mdata->type) {
	case BXXPMARRAY:
		break;
	case BXXPMFILE:
		if (mdata->stream.file != (stdout) && mdata->stream.file != (stdin))
			fclose(mdata->stream.file);
		break;
#ifdef ZPIPE
	case BXXPMPIPE:
		pclose(mdata->stream.file);
#endif
	}
}

/*
 * skip whitespace and compute the following unsigned int,
 * returns 1 if one is found and 0 if not
 */
static int xpmNextUI(bxxpmData *mdata, unsigned int *ui_return) {
	char buf[BUFSIZ];
	int l;

	l = xpmNextWord(mdata, buf);
	return atoui(buf, l, ui_return);
}

/*
 * get the current comment line
 */
static void xpmGetCmt(bxxpmData *mdata, char **cmt) {
	switch (mdata->type) {
	case BXXPMARRAY:
		*cmt = NULL;
		break;
	case BXXPMFILE:
	case BXXPMPIPE:
		if (mdata->CommentLength) {
			*cmt = (char *)malloc(mdata->CommentLength + 1);
			strncpy(*cmt, mdata->Comment, mdata->CommentLength);
			(*cmt)[mdata->CommentLength] = '\0';
			mdata->CommentLength = 0;
		}
		else
			*cmt = NULL;
		break;
	}
}

/*
 * skip to the end of the current string and the beginning of the next one
 */
static void xpmNextString(bxxpmData *mdata) {
	int c;

	switch (mdata->type) {
	case BXXPMARRAY:
		mdata->cptr = (mdata->stream.data)[++mdata->line];
		break;
	case BXXPMFILE:
	case BXXPMPIPE:
		if (mdata->Eos)
			while ((c = xpmGetC(mdata)) != mdata->Eos && c != EOF)
				;
		if (mdata->Bos) /* if not natural XPM2 */
			while ((c = xpmGetC(mdata)) != mdata->Bos && c != EOF)
				;
		break;
	}
}

/*
 * return the current character, skipping comments
 */
static int xpmGetC(bxxpmData *mdata) {
	int c;
	unsigned int n = 0, a;
	unsigned int notend;

	switch (mdata->type) {
	case BXXPMARRAY:
		return (*mdata->cptr++);
	case BXXPMFILE:
	case BXXPMPIPE:
		c = getc(mdata->stream.file);

		if (mdata->Bos && mdata->Eos && (c == mdata->Bos || c == mdata->Eos)) {
			/* if not natural XPM2 */
			mdata->InsideString = !mdata->InsideString;
			return (c);
		}
		if (!mdata->InsideString && mdata->Bcmt && c == mdata->Bcmt[0]) {
			mdata->Comment[0] = c;

			/*
			 * skip the string beginning comment
			 */
			do {
				c = getc(mdata->stream.file);
				mdata->Comment[++n] = c;
			} while (c == mdata->Bcmt[n] && mdata->Bcmt[n] != '\0' && c != EOF);

			if (mdata->Bcmt[n] != '\0') {
				/* this wasn't the beginning of a comment */
				/* put characters back in the order that we got them */
				for (a = n; a > 0; a--)
					xpmUngetC(mdata->Comment[a], mdata);
				return (mdata->Comment[0]);
			}

			/*
			 * store comment
			 */
			mdata->Comment[0] = mdata->Comment[n];
			notend = 1;
			n = 0;
			while (notend) {
				while (mdata->Comment[n] != mdata->Ecmt[0] && c != EOF) {
					c = getc(mdata->stream.file);
					mdata->Comment[++n] = c;
				}
				mdata->CommentLength = n;
				a = 0;
				do {
					c = getc(mdata->stream.file);
					n++;
					a++;
					mdata->Comment[n] = c;
				} while (c == mdata->Ecmt[a] && mdata->Ecmt[a] != '\0' && c != EOF);
				if (mdata->Ecmt[a] == '\0') {
					/* this is the end of the comment */
					notend = 0;
					xpmUngetC(mdata->Comment[n], mdata);
				}
			}
			c = xpmGetC(mdata);
		}
		return (c);
	}
	return ('\0');
}

/*
 * push the given character back
 */
static int xpmUngetC(int c, bxxpmData *mdata) {
	switch (mdata->type) {
	case BXXPMARRAY:
		return (*--mdata->cptr = c);
	case BXXPMFILE:
	case BXXPMPIPE:
		if (mdata->Bos && (c == mdata->Bos || c == mdata->Eos))
			/* if not natural XPM2 */
			mdata->InsideString = !mdata->InsideString;
		return (ungetc(c, mdata->stream.file));
	}
	return ('\0');
}

/*
 * skip whitespace and return the following word
 */
static unsigned int xpmNextWord(bxxpmData *mdata, char *buf) {
	unsigned int n = 0;
	int c;

	switch (mdata->type) {
	case BXXPMARRAY:
		c = *mdata->cptr;
		while (isspace(c) && c != mdata->Eos) {
			mdata->cptr++;
			c = *mdata->cptr;
		}
		do {
			c = *mdata->cptr++;
			buf[n++] = c;
		} while (!isspace(c) && c != mdata->Eos && c != '\0');
		n--;
		mdata->cptr--;
		break;
	case BXXPMFILE:
	case BXXPMPIPE:
		c = xpmGetC(mdata);
		while (isspace(c) && c != mdata->Eos)
			c = xpmGetC(mdata);
		while (!isspace(c) && c != mdata->Eos && c != EOF) {
			buf[n++] = c;
			c = xpmGetC(mdata);
		}
		xpmUngetC(c, mdata);
		break;
	}
	return (n);
}

static int BxXpmVisualType(Visual *visual) {
#if defined(__cplusplus) || defined(c_plusplus)
	switch (visual->c_class)
#else
	switch (visual->class)
#endif
	{
	case StaticGray:
	case GrayScale:
		switch (visual->map_entries) {
		case 2:
			return (BXMONO);
		case 4:
			return (BXGRAY4);
		default:
			return (BXGRAY);
		}
	default:
		return (BXCOLOR);
	}
}

/*
 * Free the computed color table
 */

static void xpmFreeColorTable(char ***colorTable, int ncolors) {
	int a, b;

	if (colorTable) {
		for (a = 0; a < ncolors; a++)
			if (colorTable[a]) {
				for (b = 0; b < (BXNKEYS + 1); b++)
					if (colorTable[a][b])
						free(colorTable[a][b]);
				free((char *)colorTable[a]);
			}
		free((char *)colorTable);
	}
}

#else /* USE_XPM_LIBRARY */

#include <xpm.h>

#define BxXpmColorError XpmColorError
#define BxXpmSuccess XpmSuccess
#define BxXpmOpenFailed XpmOpenFailed
#define BxXpmFileInvalid XpmFileInvalid
#define BxXpmNoMemory XpmNoMemory
#define BxXpmColorFailed XpmColorFailed

#define BxXpmVisual XpmVisual
#define BxXpmColormap XpmColormap
#define BxXpmDepth XpmDepth
#define BxXpmSize XpmSize
#define BxXpmHotspot XpmHotspot
#define BxXpmCharsPerPixel XpmCharsPerPixel
#define BxXpmColorSymbols XpmColorSymbols
#define BxXpmRgbFilename XpmRgbFilename
#define BxXpmInfos XpmInfos

#define BxXpmReturnPixels XpmReturnPixels
#define BxXpmReturnInfos XpmReturnInfos

#define BxXpmCreatePixmapFromData XpmCreatePixmapFromData
#define BxXpmCreateImageFromData XpmCreateImageFromData
#define BxXpmFreeAttributes XpmFreeAttributes

typedef XpmAttributes BxXpmAttributes;

#endif /* USE_XPM_LIBRARY */

Pixmap XPM_PIXMAP(Widget w, char **pixmapName) {
	BxXpmAttributes attributes;
	int argcnt;
	Arg args[10];
	Pixmap pixmap;
	Pixmap shape;
	int returnValue;

	argcnt = 0;
	XtSetArg(args[argcnt], XmNdepth, &(attributes.depth));
	argcnt++;
	XtSetArg(args[argcnt], XmNcolormap, &(attributes.colormap));
	argcnt++;
	XtGetValues(w, args, argcnt);

	attributes.visual = DefaultVisual(XtDisplay(w), DefaultScreen(XtDisplay(w)));
	attributes.valuemask = (BxXpmDepth | BxXpmColormap | BxXpmVisual);

	returnValue =
	    BxXpmCreatePixmapFromData(XtDisplay(w), DefaultRootWindow(XtDisplay(w)), pixmapName, &pixmap, &shape, &attributes);
	if (shape) {
		XFreePixmap(XtDisplay(w), shape);
	}

	switch (returnValue) {
	case BxXpmOpenFailed:
	case BxXpmFileInvalid:
	case BxXpmNoMemory:
	case BxXpmColorFailed:
		XtWarning("Could not create pixmap.");
		return (XmUNSPECIFIED_PIXMAP);
	default:
		return (pixmap);
	}
}

#endif

/* This structure is for capturing app-defaults values for a Class */

typedef struct _UIAppDefault {
	char *cName;     /* Class name */
	char *wName;     /* Widget name */
	char *cInstName; /* Name of class instance(nested) */
	char *wRsc;      /* Widget resource */
	char *value;     /* value read from app-defaults */
} UIAppDefault;

void setDefaultResources(char *_name, Widget w, String *resourceSpec) {
	int i;
	Display *dpy = XtDisplay(w); /* Retrieve the display pointer */
	XrmDatabase rdb = NULL;      /* A resource data base */

	/* Create an empty resource database */

	rdb = XrmGetStringDatabase("");

	/* Add the Component resources, prepending the name of the component */

	i = 0;
	while (resourceSpec[i] != NULL) {
		char buf[1000];

		sprintf(buf, "*%s%s", _name, resourceSpec[i++]);
		XrmPutLineResource(&rdb, buf);
	}

	/* Merge them into the Xt database, with lowest precendence */

	if (rdb) {
#if (XlibSpecificationRelease >= 5)
		XrmDatabase db = XtDatabase(dpy);
		XrmCombineDatabase(rdb, &db, FALSE);
#else
		XrmMergeDatabases(dpy->db, &rdb);
		dpy->db = rdb;
#endif
	}
}

/*
 * This method gets all the resources from the app-defaults file
 * (resource databse) and fills in the table (defs) if the app default
 * value exists.
 */
void InitAppDefaults(Widget parent, UIAppDefault *defs) {
	XrmQuark cQuark;
	XrmQuark rsc[10];
	XrmRepresentation rep;
	XrmValue val;
	XrmDatabase rdb;
	int rscIdx;

/* Get the database */

#if (XlibSpecificationRelease >= 5)
	if ((rdb = XrmGetDatabase(XtDisplay(parent))) == NULL) {
		return; /*  Can't get the database */
	}
#else
	Display *dpy = XtDisplay(parent);
	if ((rdb = dpy->db) == NULL) {
		return;
	}
#endif

	/* Look for each resource in the table */

	while (defs->wName) {
		rscIdx = 0;

		cQuark = XrmStringToQuark(defs->cName); /* class quark */
		rsc[rscIdx++] = cQuark;
		if (defs->wName[0] == '\0') {
			rsc[rscIdx++] = cQuark;
		}
		else {
			if (strchr(defs->wName, '.') == NULL) {
				rsc[rscIdx++] = XrmStringToQuark(defs->wName);
			}
			else {
				/*
				 * If we found a '.' that means that this is not
				 * a simple widget name, but a sub specification so
				 * we need to split this into several quarks.
				 */
				char *copy = XtNewString(defs->wName), *ptr;

				for (ptr = strtok(copy, "."); ptr != NULL; ptr = strtok(NULL, ".")) {
					rsc[rscIdx++] = XrmStringToQuark(ptr);
				}
				XtFree(copy);
			}
		}

		if (defs->cInstName && defs->cInstName[0] != '\0') {
			rsc[rscIdx++] = XrmStringToQuark(defs->cInstName);
		}

		rsc[rscIdx++] = XrmStringToQuark(defs->wRsc);
		rsc[rscIdx++] = NULLQUARK;

		if (XrmQGetResource(rdb, rsc, rsc, &rep, &val)) {
			defs->value = XtNewString((char *)val.addr);
		}
		defs++;
	}
}

/*
 * This method applies the app defaults for the class to a specific
 * instance. All the widgets in the path are loosly coupled (use *).
 * To override a specific instance, use a tightly coupled app defaults
 * resource line (use .).
 */
void SetAppDefaults(
    Widget w, UIAppDefault *defs, char *inst_name, Boolean override_inst) {
	Display *dpy = XtDisplay(w); /*  Retrieve the display */
	XrmDatabase rdb = NULL;      /* A resource data base */
	char lineage[1000];
	char buf[1000];
	Widget parent;

	/* Protect ourselves */

	if (inst_name == NULL)
		return;

	/*  Create an empty resource database */

	rdb = XrmGetStringDatabase("");

	/* Start the lineage with our name and then get our parents */

	lineage[0] = '\0';
	parent = w;

	while (parent) {
		WidgetClass wclass = XtClass(parent);

		if (wclass == applicationShellWidgetClass)
			break;

		strcpy(buf, lineage);
		sprintf(lineage, "*%s%s", XtName(parent), buf);

		parent = XtParent(parent);
	}

	/*  Add the Component resources, prepending the name of the component */
	while (defs->wName != NULL) {
		int name_length;
		/*
		 * We don't deal with the resource if it isn't found in the
		 * Xrm database at class initializtion time (in initAppDefaults).
		 * Special handling of class instances.
		 */
		if (strchr(defs->wName, '.')) {
			name_length = strlen(defs->wName) - strlen(strchr(defs->wName, '.'));
		}
		else {
			name_length = strlen(defs->wName) > strlen(inst_name) ? strlen(defs->wName) : strlen(inst_name);
		}
		if (defs->value == NULL || (override_inst && strncmp(inst_name, defs->wName, name_length)) ||
		    (!override_inst && defs->cInstName != NULL)) {
			defs++;
			continue;
		}

		/* Build up string after lineage */
		if (defs->cInstName != NULL) {
			/* Don't include class instance name if it is also the instance */
			/* being affected.  */

			if (*defs->cInstName != '\0') {
				sprintf(buf, "%s*%s*%s.%s: %s", lineage, defs->wName, defs->cInstName, defs->wRsc, defs->value);
			}
			else {
				sprintf(buf, "%s*%s.%s: %s", lineage, defs->wName, defs->wRsc, defs->value);
			}
		}
		else if (*defs->wName != '\0') {
			sprintf(buf, "%s*%s*%s.%s: %s", lineage, inst_name, defs->wName, defs->wRsc, defs->value);
		}
		else {
			sprintf(buf, "%s*%s.%s: %s", lineage, inst_name, defs->wRsc, defs->value);
		}

		XrmPutLineResource(&rdb, buf);
		defs++;
	}

	/* Merge them into the Xt database, with lowest precendence */
	if (rdb) {
#if (XlibSpecificationRelease >= 5)
		XrmDatabase db = XtDatabase(dpy);
		XrmCombineDatabase(rdb, &db, FALSE);
#else
		XrmMergeDatabases(dpy->db, &rdb);
		dpy->db = rdb;
#endif
	}
}
