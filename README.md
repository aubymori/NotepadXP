This is a modified version of Windows Notepad, based on the original source code for
Notepad. It adds several features of the Windows 10 notepad that aren't available
for users of earlier versions of Windows, as well as some new features and enhancements:

- Unix (LF) line ending support
- Option to change the tab width from the default of 8 to either 2 or 4
- Allow deleting a word using Ctrl+Backspace or Ctrl+Delete
- Eliminate the well-known "Bush hid the facts" glitch
- Add Ctrl+Shift+S shortcut to open the Save As menu
- Add Ctrl+Shift+N shortcut to open a new Notepad window
- Add an asterisk indicator in the title bar to indicate unsaved changes
- Add "Find Previous" menu item
- Improved search features: "Whole word only" and "Wrap around"
- Search options are memorized across searches and app restarts
- Allow Word Wrap + Status Bar together
- Allow "Goto..." function when word wrap is enabled
- Cascade new Notepad windows when an existing instance of Notepad is open

NotepadEx may replace your system Notepad - simply take ownership of, and replace the
notepad.exe from %windir%, %windir%\System32, and %windir%\SysWOW64 (if applicable).

Here is an example screenshot of how it works, which is essentially the same as how it
does in Windows 10.

![Example screenshot](https://raw.githubusercontent.com/vxiiduu/NotepadEx/main/screenshot.png)

Additionally to the Unix line endings support, NotepadEx converts the XP code-base to
use Vista/7 features, such as task dialogs, common file dialogs (aka "new style open dialog"),
and new style help interface. The old interfaces are used when running on Windows 2000 or XP,
since this software maintains full backwards compatibility with NT 5.x-based OSes.
