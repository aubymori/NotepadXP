/*
 * misc notepad functions
 * Copyright (C) 1984-2000 Microsoft Corporation
 */

#include "precomp.h"

BOOL fCase = FALSE;         // Flag specifying case sensitive search 
BOOL fReverse = FALSE;      // Flag for direction of search 
BOOL fWholeWord = FALSE;	// Match whole word only
BOOL fWrapAround = FALSE;	// Wrap around

extern HWND hDlgFind;       // handle to modeless FindText window 

BOOL IsBreakChar(WCHAR wch); // defined in notepad.c

// Search for needle in the haystack from the end to the beginning.
LPTSTR ReverseScan(
	IN		LPTSTR	lpHaystack,
	IN		LPTSTR	lpHaystackEnd OPTIONAL,
	IN OUT	LPTSTR	lpNeedle,
	IN		BOOL	fCaseSensitive)
{
	TCHAR cLastCharU;
	TCHAR cLastCharL;
	ULONG NeedleLength;

	cLastCharU = (TCHAR) (INT_PTR) CharUpper((LPTSTR)(INT_PTR)(*lpNeedle));
	cLastCharL = (TCHAR) (INT_PTR) CharLower((LPTSTR)(INT_PTR)(*lpNeedle));

	NeedleLength = lstrlen(lpNeedle);

	if (!lpHaystackEnd) {
		lpHaystackEnd = lpHaystack + lstrlen(lpHaystack);
	}

	do {
		// NotepadEx fix: Change == to <= to make the code more robust
		// in the case of invalid parameters. This isn't actually
		// required, though. It just catches certain types of coding
		// errors that I may or may not have introduced during development.
		if (lpHaystackEnd <= lpHaystack) {
			return NULL;
		}

		--lpHaystackEnd;

		if (fCaseSensitive) {
			if (*lpHaystackEnd != *lpNeedle) {
				continue;
			}
		} else {
			if(!(*lpHaystackEnd == cLastCharU || *lpHaystackEnd == cLastCharL)) {
				continue;
			}
		}

		if (fCaseSensitive) {
			if (!_tcsncmp(lpHaystackEnd, lpNeedle, NeedleLength)) {
				break;
			}
		} else {
			//
			// compare whole string using locale specific comparison.
			// do not use C runtime version since it may be wrong.
			//

			if (CompareString(LOCALE_USER_DEFAULT,
							  NORM_IGNORECASE | SORT_STRINGSORT | NORM_STOP_ON_NULL,
							  lpHaystackEnd, NeedleLength,
							  lpNeedle, NeedleLength) == 2) {
				break;
			}
		}
	} while (TRUE);

	if (fWholeWord) {
		// Check that the preceding and following characters, if any,
		// are word-break characters. If not, we will keep searching.
		// We know that if we reach this point:
		//   - the needle is inside the haystack
		//   - the needle is preceded by at least one character
		//   - the haystack is suffixed by a zero byte.
		// Therefore it is safe to read one byte past the end of
		// the needle.

		if (lpHaystackEnd != lpHaystack && !IsBreakChar(lpHaystackEnd[-1])) {
			// no preceding character, or preceding character is not word break
			return NULL;
		}

		if (lpHaystackEnd[NeedleLength] != '\0' && !IsBreakChar(lpHaystackEnd[NeedleLength])) {
			// no following character, or following character is not word break
			return NULL;
		}
	}

	return lpHaystackEnd;
}

// NotepadEx modification:
// lpHaystackBegin = the beginning of the source buffer
// lpHaystackSearch = the beginning of the search
LPTSTR ForwardScan(LPTSTR lpHaystackBegin, LPTSTR lpHaystackSearch, LPTSTR lpNeedle, BOOL fCaseSensitive)
{
	BOOL fAlreadyWrappedAround;
	BOOL fFound;
	LPTSTR lpHaystackSearchEnd;
	TCHAR cFirstCharU;
	TCHAR cFirstCharL;
	ULONG NeedleLength = lstrlen(lpNeedle);

	fFound = FALSE;
	fAlreadyWrappedAround = FALSE;
	lpHaystackSearchEnd = lpHaystackSearch;
	cFirstCharU= (TCHAR) (INT_PTR) CharUpper( (LPTSTR)(INT_PTR)(*lpNeedle) );
	cFirstCharL= (TCHAR) (INT_PTR) CharLower( (LPTSTR)(INT_PTR)(*lpNeedle) );

SearchAgain:
	while (*lpHaystackSearch && (!fAlreadyWrappedAround || lpHaystackSearch + 1 != lpHaystackSearchEnd)) {
		if (fCaseSensitive) {
			if (*lpHaystackSearch != *lpNeedle) {
				lpHaystackSearch++;
				continue;
			}
		} else {
			if (!(*lpHaystackSearch == cFirstCharU || *lpHaystackSearch == cFirstCharL)) {
				lpHaystackSearch++;
				continue;
			}
		}

		if (fCaseSensitive) {
			if (!_tcsncmp(lpHaystackSearch, lpNeedle, NeedleLength)) {
				fFound = TRUE;
			}
		} else {
			if (CompareString(	LOCALE_USER_DEFAULT,
				NORM_IGNORECASE | SORT_STRINGSORT | NORM_STOP_ON_NULL,
				lpHaystackSearch, NeedleLength,
				lpNeedle, NeedleLength) == 2) {
					fFound = TRUE;
			}
		}

		if (fFound && fWholeWord) {
			BOOL fReallyFound;
			// see the equivalent part of ReverseScan

			fReallyFound = FALSE;

			if ((lpHaystackSearch == lpHaystackBegin || IsBreakChar(lpHaystackSearch[-1])) &&
			    (IsBreakChar(lpHaystackSearch[NeedleLength]) ||
			     lpHaystackSearch[NeedleLength] == '\0')) {
				fReallyFound = TRUE;
			}

			if (fReallyFound) {
				break;
			}

			fFound = FALSE;
		} else if (fFound) {
			break;
		}

		lpHaystackSearch++;
	}

	if (fWrapAround && !fFound && !fAlreadyWrappedAround) {
		fAlreadyWrappedAround = TRUE;
		lpHaystackSearch = lpHaystackBegin;
		goto SearchAgain;
	}

	return fFound ? lpHaystackSearch : NULL;
}

// search forward or backward in the edit control text for the given pattern
// It is the responsibility of the caller to set the cursor
BOOL Search (TCHAR * szKey)
{
    BOOL      bStatus= FALSE;
    TCHAR   * pStart, *pMatch;
    DWORD     StartIndex, OriginalStartIndex, OriginalEndIndex, LineNum, TotalLineCount, EndIndex;
    DWORD     SelStart, SelEnd, i;
    HANDLE    hEText;           // handle to edit text
    UINT      uSelState;
    HMENU     hMenu;
    BOOL      bSelectAll = FALSE;
	BOOL	  fAlreadyWrappedAround = FALSE;


    if (!*szKey)
        return( bStatus );

    SendMessage(hwndEdit, EM_GETSEL, (WPARAM)&SelStart, (LPARAM)&SelEnd);


    // when we finish the search, we highlight the text found, and continue 
    // the search after the end of the highlighted position (in forward 
    // case) or from the begining of the highlighted position in the reverse
    // direction (in reverse case). this would break if the user has 
    // selected all text. this hack would take care of it. (this is consistent
    // with VC editors' search too.

    hMenu = GetMenu(hwndNP);
    uSelState = GetMenuState(GetSubMenu(hMenu, 1), M_SELECTALL, MF_BYCOMMAND);
    if (uSelState == MF_GRAYED)
    {
        bSelectAll = TRUE;
        SelStart = SelEnd =0;
    }


    //
    // get pointer to edit control text to search
    //

    hEText= (HANDLE) SendMessage( hwndEdit, EM_GETHANDLE, 0, 0 );
    if( !hEText )  // silently return if we can't get it
    {
        return( bStatus );
    }

	pStart = (PTCHAR) LocalLock( hEText );

	if( !pStart ) {
		return( bStatus );
	}

	if (fReverse) {
		// Get current line number 
		LineNum = (DWORD)SendMessage(hwndEdit, EM_LINEFROMCHAR, SelStart, 0);
		TotalLineCount = Edit_GetLineCount(hwndEdit);

		// Get index to start of the line
		StartIndex = (DWORD)SendMessage(hwndEdit, EM_LINEINDEX, LineNum, 0);
		OriginalStartIndex = StartIndex;

		// Set upper limit for search text
		EndIndex = SelStart;
		OriginalEndIndex = EndIndex;

		pMatch = NULL;

		// Search line by line, from LineNum to 0
		i = LineNum;

		while (TRUE) {
			pMatch = ReverseScan(pStart + StartIndex, pStart + EndIndex, szKey, fCase);

			if (pMatch) {
				break;
			}

			// current StartIndex is the upper limit for the next search 
			EndIndex = StartIndex;

			if ((!fAlreadyWrappedAround && i != 0) || (fAlreadyWrappedAround && i != LineNum)) {
				// Get start of the next line
				i--;
				StartIndex = (DWORD)SendMessage(hwndEdit, EM_LINEINDEX, i, 0);
			} else if (fWrapAround && !fAlreadyWrappedAround) {
				// NotepadEx
				fAlreadyWrappedAround = TRUE;
				i = TotalLineCount - 1;											// wrap around to last line
				StartIndex = Edit_LineIndex(hwndEdit, i);						// beginning of last line

				if (StartIndex == -1) {
					// I'm not sure if this can happen in the real world.
					// -1 is returned when the line 'i' doesn't exist in the edit control.
					// I suspect that this could happen in a race condition if the user
					// modifies stuff while a search is occurring, so I'm going to put
					// this check here.
					break;
				}

				EndIndex = StartIndex + Edit_LineLength(hwndEdit, StartIndex);	// end of last line
			} else {
				break;
			}
		}
	} else {
		pMatch = ForwardScan(pStart, pStart+SelEnd, szKey, fCase);
	}

    LocalUnlock(hEText);

    if (pMatch == NULL)
    {
        //
        // alert user on not finding any text unless it is replace all
        //
        if( !(FR.Flags & FR_REPLACEALL) )
        {
            HANDLE hPrevCursor= SetCursor( hStdCursor );
            AlertBox( hDlgFind ? hDlgFind : hwndNP,
                      szNN,
                      szCFS,
                      szSearch,
                      MB_APPLMODAL | MB_OK | MB_ICONINFORMATION);
            SetCursor( hPrevCursor );
        }
    }
    else
    {
        SelStart = (DWORD)(pMatch - pStart);
        SendMessage( hwndEdit, EM_SETSEL, SelStart, SelStart+lstrlen(szKey));

        // since we are selecting the found text, enable SelectAll again.
        if (bSelectAll)
        {
            EnableMenuItem(GetSubMenu(hMenu, 1), M_SELECTALL, MF_ENABLED);
        }

        //
        // show the selected text unless it is replace all
        //

        if( !(FR.Flags & FR_REPLACEALL) )
        {
            SendMessage(hwndEdit, EM_SCROLLCARET, 0, 0);
            UpdateStatusBar( FALSE );
        }
        bStatus= TRUE;   // found
    }

    return( bStatus );
}

// Recreate notepad edit window, get text from old window and put in new window. 
// Called when user changes style from wrap on/off 
//
// Called with the style of the new window
//

BOOL NpReCreate( long style )
{
    RECT    rcT1;
    HWND    hwndT1;
    HANDLE  hT1;
    int     cchTextNew;
    TCHAR*  pchText;
    BOOL    fWrapIsOn = ((style & WS_HSCROLL) != 0);
    HCURSOR hPrevCursor;
    BOOL    bModified;     // modify flag from old edit buffer

	ULONG	SelectionStart;	// NotepadEx: save selection and cursor pos.
	ULONG	SelectionEnd;	// when entering/leaving wordwrap

	// NotepadEx: save old cursor position
	SendMessage(hwndEdit, EM_GETSEL, (WPARAM) &SelectionStart, (LPARAM) &SelectionEnd);

    // if wordwrap, remove soft carriage returns 
    hPrevCursor= SetCursor( hWaitCursor );     // this may take some time...
    if( fWrapIsOn ) 
    {
        GotoAndScrollInView(1);  // get around MLE bug
        SendMessage(hwndEdit, EM_FMTLINES, FALSE, 0L);
    }

    bModified= (SendMessage( hwndEdit, EM_GETMODIFY, 0,0 ) != 0);

    cchTextNew= (int)SendMessage( hwndEdit, WM_GETTEXTLENGTH, 0, 0L );
    hT1= LocalAlloc( LMEM_MOVEABLE, ByteCountOf(cchTextNew + 1) );
    if( !hT1 )
    {
        // failed, restore wordwrap; insert soft carriage returns
        if( fWrapIsOn )
        {
            SendMessage(hwndEdit, EM_FMTLINES, TRUE, 0L);
        }
        SetCursor( hPrevCursor );
        return FALSE;
    }

    GetClientRect( hwndNP, (LPRECT)&rcT1 );

    //
    // save the current edit control text.
    //

    pchText= LocalLock (hT1);
    SendMessage( hwndEdit, WM_GETTEXT, cchTextNew+1, (LPARAM)pchText );
    hwndT1= CreateWindowEx( WS_EX_CLIENTEDGE,
        TEXT("Edit"),
        TEXT(""), // pchText
        style,
        0,
        0,
        rcT1.right,
        rcT1.bottom,
        hwndNP,
        (HMENU)ID_EDIT,
        hInstanceNP, NULL );
    if( !hwndT1 )
    {
        SetCursor( hPrevCursor );
        if( fWrapIsOn )      // restore wordwrap
        {
            SendMessage( hwndEdit, EM_FMTLINES, TRUE, 0L );
        }
        LocalUnlock(hT1);
        LocalFree(hT1);
        return FALSE;
    }

    //
    // The user can "add" styles to the edit window after it is
    // created (like WS_EX_RTLREADING) when language packs are installed.
    // Preserve these styles when changing the word wrap.
    //

    SetWindowLong( hwndT1 ,
                   GWL_EXSTYLE ,
                   GetWindowLong( hwndEdit , GWL_EXSTYLE )|WS_EX_CLIENTEDGE ) ;

    // Set font before set text to save time calculating
    SendMessage( hwndT1, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0) );

	// NotepadEx: Restore tab size before setting text
	Edit_SetTabStops(hwndT1, 1, &iTabStops);

    if (!SendMessage (hwndT1, WM_SETTEXT, 0, (LPARAM) pchText))
    {
        SetCursor( hPrevCursor );
        if( fWrapIsOn )   // restore wordwrap
        {
            SendMessage( hwndEdit, EM_FMTLINES, TRUE, 0L );
        }
        DestroyWindow( hwndT1 );
        LocalUnlock( hT1 );
        LocalFree( hT1 );
        return FALSE;
    }
    LocalUnlock(hT1);


    DestroyWindow( hwndEdit );     // out with the old
    hwndEdit = hwndT1;             // in with the new

    // free the earlier allocated memory in hEdit

    if (hEdit)
        LocalFree(hEdit);

    hEdit = hT1;

    // limit text for safety's sake.
    PostMessage( hwndEdit, EM_LIMITTEXT, (WPARAM)CCHNPMAX, 0L );

	// NotepadEx: restore old cursor pos. and scroll back to where we can see it
	Edit_SetSel(hwndEdit, SelectionStart, SelectionEnd);
	Edit_ScrollCaret(hwndEdit);
	Edit_Scroll(hwndEdit, 5, 0);

    ShowWindow(hwndNP, SW_SHOW);
    SendMessage( hwndEdit, EM_SETMODIFY, bModified, 0L );
    SetFocus(hwndEdit);

    SetCursor( hPrevCursor );   // restore cursor

    // redraw the status bar

    if( fStatus )
    {
        RECT rcClient;
        GetClientRect(hwndNP, &rcClient);
        NPSize(rcClient.right - rcClient.left, rcClient.bottom - rcClient.top);
        UpdateStatusBar( TRUE );
        ShowWindow( hwndStatus, SW_SHOW );
   }

    return TRUE;
}

// NotepadEx addition.
// Called when Ctrl+Shift+N is pressed or the File->New Window
// menu item is clicked.
VOID NewWindow(
	VOID)
{
	WCHAR PathToNotepad[MAX_PATH];
	STARTUPINFO StartupInfo;
	PROCESS_INFORMATION ProcessInformation;
	ULONG Success;
	
	//
	// Start a new notepad process with CreateProcess.
	//

	Success = GetModuleFileName(NULL, PathToNotepad, ARRAYSIZE(PathToNotepad));

	if (!Success) {
		return;
	}

	GetStartupInfo(&StartupInfo);

	Success = CreateProcess(
		PathToNotepad,
		NULL,
		NULL,
		NULL,
		FALSE,
		0,
		NULL,
		NULL,
		&StartupInfo,
		&ProcessInformation);

	if (!Success) {
		return;
	}

	CloseHandle(ProcessInformation.hProcess);
	CloseHandle(ProcessInformation.hThread);
}