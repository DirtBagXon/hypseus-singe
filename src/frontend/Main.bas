Attribute VB_Name = "Main"
'DaphneLoader - a simple Windows front-end GUI for the Daphne emulator
' (feel free to write a better one!)
' By Warren Ondras
' warren@dragons-lair-project.com
' http://daphne.rulecity.com



Dim InputFile As String
Dim GameListFile As String
Dim ConfigFile As String

Dim CurrentGame As String

Public keyArray(3, MaxKeys) As Variant

'for launching external files in registered applications
Private Declare Function ShellExecute Lib _
     "shell32.dll" Alias "ShellExecuteA" _
     (ByVal hWnd As Long, ByVal lpOperation _
     As String, ByVal lpFile As String, ByVal _
     lpParameters As String, ByVal lpDirectory _
     As String, ByVal nShowCmd As Long) As Long


Function init() As Boolean
    Dim thisName As String
    Dim tmpstr As String
    Dim LastGame As String
   
    InputFile = App.Path & "\" & KeyMapFile
    GameListFile = App.Path & "\DaphneLoaderInfo.ini"
    ConfigFile = App.Path & "\DaphneLoaderPrefs.ini"

    LoadKeyArrays
    
    ' get key bindings
    For i = 1 To MaxKeys
        thisName = GetFromINI("InputKeys", "Key" & i & "Setting", GetGameListFile)
        tmpstr = Trim(GetFromINI("KEYBOARD", thisName, InputFile))
    
        If tmpstr = "" Then 'if no file or not defined, get the default
            keyArray(0, i - 1) = GetIntFromINI("InputKeys", "Key" & i & "Default1", 0, GameListFile)
            keyArray(1, i - 1) = GetIntFromINI("InputKeys", "Key" & i & "Default2", 0, GameListFile)
            keyArray(2, i - 1) = GetIntFromINI("InputKeys", "Button" & i & "Default1", 0, GameListFile)
        Else
            tmpArray = Split(tmpstr, " ", 3)
            If (UBound(tmpArray) >= 0) Then keyArray(0, i - 1) = tmpArray(0)
            If (UBound(tmpArray) >= 1) Then keyArray(1, i - 1) = tmpArray(1)
            If (UBound(tmpArray) >= 2) Then keyArray(2, i - 1) = tmpArray(2)
            
            If (keyArray(0, i - 1) = "") Then keyArray(0, i - 1) = 0
            'MsgBox keyArray(0, i - 1)
            If (keyArray(1, i - 1) = "") Then keyArray(1, i - 1) = 0
            If (keyArray(2, i - 1) = "") Then keyArray(2, i - 1) = 0
        End If
    Next i

    LastGame = GetFromINI("General", "Lastgame", GetConfigFile)
    
    'populate the game list
    numberofgames = Val(GetFromINI("GameList", "NumberOfGames", GameListFile))
    For i = 1 To numberofgames
        tmpstr = GetFromINI("GameList", "Game" & i, GameListFile)
        MainForm.GameList.AddItem (tmpstr)
        'highlight last selected game of previous session if valid, otherwise the first in the list
        If (i = 1) Or (tmpstr = LastGame) Then MainForm.GameList.ListIndex = i - 1
    Next
    
    'recall the last window size and position
    tmpval = GetIntFromINI("General", "WindowTop", 0, GetConfigFile)
    If tmpval > 0 Then MainForm.Top = tmpval
    tmpval = GetIntFromINI("General", "WindowLeft", 0, GetConfigFile)
    If tmpval > 0 Then MainForm.Left = tmpval
    tmpval = GetIntFromINI("General", "WindowWidth", 0, GetConfigFile)
    If tmpval > 0 Then MainForm.Width = tmpval
    tmpval = GetIntFromINI("General", "WindowHeight", 0, GetConfigFile)
    If tmpval > 0 Then MainForm.Height = tmpval
    
    
End Function

Sub SetCurrentGame(GameName As String)
    CurrentGame = GameName
    
    MainForm.GamePicBox.Picture = LoadPicture()
    
    picname = GetFromINI(CurrentGame, "imagename", GameListFile)
    
    If Len(picname) > 0 Then
        PicFile = App.Path & "\images\" & picname
        If Len(Dir(PicFile)) > 0 Then MainForm.GamePicBox.Picture = LoadPicture(PicFile)
    End If
    
End Sub


Sub StartGame()
    CmdLine = GetCmdLine(CurrentGame)
    If CmdLine = "" Then Exit Sub 'error or user cancelled
    
    If Len(Dir(App.Path & "\daphne.exe")) > 0 Then
        ChDir (App.Path) 'the mpeg browse button seems to make us lose this...
        ChDrive (Left$(App.Path, 1)) '...and this too...
        
      Shell (App.Path & "\daphne.exe " & CmdLine)
      '  ShellExecute 0&, vbNullString, App.Path & "daphne.exe " & CmdLine, vbNullString, _
      '      vbNullString, 1 ' 1=SW_SHOWNORMAL
    
    Else
        MsgBox "Can't find DAPHNE.EXE -- must be in same directory as Daphne Loader.", vbOKOnly + vbCritical, "Daphne Loader Error"
    End If
End Sub


Sub LaunchDaphneURL()
    'take me to the Daphne web site!
    'Shell "explorer http://daphne.rulecity.com", vbNormalFocus   'launches in IE
    ShellExecute 0&, vbNullString, "http://www.daphne-emu.com", vbNullString, vbNullString, 1  ' 1=SW_SHOWNORMAL
End Sub

Sub ViewErrorLog()
    logfile = App.Path & "\daphne_log.txt"
    
    If Len(Dir(logfile)) > 0 Then
        ShellExecute 0&, vbNullString, logfile, vbNullString, _
        vbNullString, 1 ' 1=SW_SHOWNORMAL
    Else
        MsgBox "Can't find Daphne error log (daphne_log.txt) -- must be in same directory as Daphne Loader.", vbOKOnly + vbCritical, "Daphne Loader Error"
    End If
End Sub

Sub LaunchDocs()
   ShellExecute 0&, vbNullString, "http://www.daphne-emu.com/docs.html", vbNullString, vbNullString, 1 ' 1=SW_SHOWNORMAL
'    docfile = App.Path & "\doc\docs.html"
    
'    If Len(Dir(docfile)) > 0 Then
'        ShellExecute 0&, vbNullString, docfile, vbNullString, _
'        vbNullString, 1 ' 1=SW_SHOWNORMAL
'    Else
'        MsgBox "Can't find Daphne Documentation -- " & docfile, vbOKOnly + vbCritical, "Daphne Loader Error"
'    End If
End Sub
Sub EditFile(FileName As String)
    If Len(FileName) > 0 Then
        Shell "notepad.exe " & FileName, vbNormalFocus
        
    Else
        MsgBox "Invalid path/filename specified." & docfile, vbOKOnly + vbCritical, "Daphne Loader Error"
    End If


End Sub
Sub ExitApp()
    ' save the currently selected game, so we can return to it on startup next time
    SaveToINI "General", "LastGame", CurrentGame, ConfigFile
    
    'save the current window position and size
    ' WO: only do this if the window isn't minimized or maximized
    If MainForm.WindowState = vbNormal Then
        SaveToINI "General", "WindowTop", MainForm.Top, ConfigFile
        SaveToINI "General", "WindowLeft", MainForm.Left, ConfigFile
        SaveToINI "General", "WindowWidth", MainForm.Width, ConfigFile
        SaveToINI "General", "WindowHeight", MainForm.Height, ConfigFile
    End If
    
    ' no other cleanup needed AFAIK
    Unload MainForm
End Sub
Function GetCurrentGame() As String
    GetCurrentGame = CurrentGame
End Function
Public Function GetGameListFile() As String
  GetGameListFile = GameListFile
End Function
Function GetConfigFile() As String
  GetConfigFile = ConfigFile
End Function

