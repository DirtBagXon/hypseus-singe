Attribute VB_Name = "KeyStuff"
Public Const MaxKeys = 19
Public Const KeyMapFile = "dapinput.ini"
Public Const MaxKeyCodes = 321

Public keynamearray(MaxKeyCodes, 2) As String 'text names and keycodes for all possible keys
Public keycodearray(MaxKeyCodes) As Integer 'maps keycode to position in list
Public numdefinedkeys As Integer


'Sub ConfigureKeys()

'    Dim tmp, i, j, key1, key2 As Integer
'    Dim tmpArray
'
'
'    'fill the current list from dapinput.ini
'    For i = 1 To MaxKeys
'
'        For j = 1 To numdefinedkeys
'            KeyMapForm.KeyList(i - 1).AddItem (keynamearray(j - 1, 1))
'            KeyMapForm.KeyList2(i - 1).AddItem (keynamearray(j - 1, 1))
'        Next j
'
'        'get the display name
'        KeyMapForm.KeyLabel(i - 1).Caption = GetFromINI("InputKeys", "Key" & i & "Name", GetGameListFile)
'
'        tmpstr = Trim(GetFromINI("KEYBOARD", GetFromINI("InputKeys", "Key" & i & "Setting", GetGameListFile), InputFile))
'
'        If tmpstr = "" Then 'if no file or not defined, get the default
'            key1 = GetIntFromINI("InputKeys", "Key" & i & "Default1", 0, GetGameListFile)
'            key2 = GetIntFromINI("InputKeys", "Key" & i & "Default2", 0, GetGameListFile)
'        Else
'            tmpArray = Split(tmpstr, " ", 3)
'            key1 = tmpArray(0)
'            key2 = tmpArray(1)
'
'            MsgBox UBound(tmpArray)
''            For j = 1 To Len(tmpstr)  'this will bomb if the values are invalid!!!
' '               If Mid(tmpstr, j, 1) = " " Then
'  '                  key1 = Val(Left$(tmpstr, j - 1))
'   '                 key2 = Val(Right(tmpstr, Len(tmpstr) - j))
'    '                Exit For
'     '           End If
'      '      Next j
'        End If
'
'        '**** set the first and second keys in the listboxes
'        KeyMapForm.KeyList(i - 1).ListIndex = keycodearray(key1)
'        KeyMapForm.KeyList2(i - 1).ListIndex = keycodearray(key2)
'
'    Next i
'
'    'now show the form and let it take over!
'
'
'End Sub


'Sub ResetKeys()
'    Dim key1, key2, i As Integer
'
'    For i = 1 To MaxKeys
'        key1 = GetIntFromINI("InputKeys", "Key" & i & "Default1", 0, GetGameListFile)
'        key2 = GetIntFromINI("InputKeys", "Key" & i & "Default2", 0, GetGameListFile)
'        KeyMapForm.KeyList(i - 1).ListIndex = keycodearray(key1)
'        KeyMapForm.KeyList2(i - 1).ListIndex = keycodearray(key2)
'    Next i
'End Sub

Sub UpdateKeys()
    'write a new keymap file
    Open KeyMapFile For Output As 1
    
    Print #1, "[KEYBOARD]"
    For i = 1 To MaxKeys
        Print #1, _
            GetFromINI("InputKeys", "Key" & i & "Setting", GetGameListFile) & " = " & _
            keyArray(0, i - 1) & " " & _
            keyArray(1, i - 1) & " " & _
            keyArray(2, i - 1)
    Next i
    
    Print #1, "END"
    
    Close 1
End Sub


Sub LoadKeyArrays()
    Dim t As String
    Dim indexpos As Integer
    
    indexpos = 0
    
    For i = 0 To MaxKeyCodes
        Select Case i
        Case 0:
            t = "(none)"
        Case 8:
            t = "Backspace"
        Case 9:
            t = "Tab"
        Case 12:
            t = "Clear"
        Case 13:
            t = "Enter"
        Case 19:
            t = "Pause"
        Case 27:
            t = "ESC"
        Case 32:
            t = "Space"
        Case 39:
            t = "'"
        Case 44:
            t = ","
        Case 45:
            t = "-"
        Case 46:
            t = "."
        Case 47:
            t = "/"
        Case 48 To 57:   ' 0 to 9
             t = Chr$(i)
        Case 59:
            t = ";"
        Case 61:
            t = "="
        Case 91:
            t = "["
        Case 92:
            t = "]"
        Case 96:
            t = "`"
        Case 97 To 122:   ' A to Z
            t = Chr$(i - 32)
        Case 127:
            t = "Del"
        Case 256 To 265:
            t = "Numpad " & Chr$(i - 208)
        Case 266:
            t = "Numpad ."
        Case 267:
            t = "Numpad /"
        Case 268:
            t = "Numpad *"
        Case 269:
            t = "Numpad -"
        Case 270:
            t = "Numpad +"
        Case 271:
            t = "Numpad Enter"
        Case 272:
            t = "Numpad ="
        Case 273:
            t = "Up"
        Case 274:
            t = "Down"
        Case 275:
            t = "Right"
        Case 276:
            t = "Left"
        Case 277:
            t = "Ins"
        Case 278:
            t = "Home"
        Case 279:
            t = "End"
        Case 280:
            t = "PgUp"
        Case 281:
            t = "PgDn"
        Case 282 To 296:
            t = "F" & LTrim(Str(i - 281))
        Case 300:
            t = "NumLock"
        Case 301:
            t = "CapsLock"
        Case 302:
            t = "ScrollLock"
        Case 303:
            t = "RShift"
        Case 304:
            t = "LShift"
        Case 305:
            t = "RCtl"
        Case 306:
            t = "LCtl"
        Case 307:
            t = "RAlt"
        Case 308:
            t = "LAlt"
        Case 311:
            t = "LWin"
        Case 312:
            t = "RWin"
        Case 317:
            t = "SysRq"
        Case 305:
            t = "Break"
        Case Else
            t = ""
        End Select
        
        If Len(t) > 0 Then
            keynamearray(indexpos, 1) = t  'key name
            keynamearray(indexpos, 2) = i  'key code
            keycodearray(i) = indexpos 'position in list for this key
            indexpos = indexpos + 1
        Else
            keycodearray(i) = -1 'not in list
        End If
    Next i
   
    numdefinedkeys = indexpos
End Sub

