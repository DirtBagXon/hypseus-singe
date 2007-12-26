Attribute VB_Name = "ConfigStuff"
Public Const MaxCustomOptions = 6
Public Const MaxDIPListBoxes = 7
Public Const MaxDIPCheckBoxes = 7
Public Const MaxDIPBanks = 4
Public Const MaxCustomCheckBoxes = 2



'
'ConfigureGame()
'loads config UI with stuff
'
Sub ConfigureGame()
Dim tmpstr As String
Dim itemname As String


tmpstr = GetFromINI("General", "MaxLatency", GetGameListFile)
If Len(tmpstr) > 0 Then
    MaxLatency = Val(tmpstr)
    frmOptions.LatencySlider.Max = MaxLatency
    frmOptions.LatencySlider.TickFrequency = Int(MaxLatency / 10)
    frmOptions.LatencySlider.SmallChange = Int(MaxLatency / 10)
    frmOptions.LatencySlider.LargeChange = Int(MaxLatency / 4)
End If
    
    
'parse options and custom settings for current game

If GetBoolFromINI(GetCurrentGame, "Fullscreen", GetConfigFile) Then frmOptions.FullscreenCheckbox.Value = 1

'if there are custom options for this game, get them and populate the UI
For i = 1 To MaxCustomOptions
tmpstr = GetFromINI(GetCurrentGame, "ListBox" & i & "Title", GetGameListFile)
If Len(tmpstr) > 0 Then

    numchoices = GetIntFromINI(GetCurrentGame, "ListBox" & i & "Items", 1, GetGameListFile)
    For j = 1 To numchoices
        itemname = GetFromINI(GetCurrentGame, "ListBox" & i & "ItemName" & j, GetGameListFile)
        frmOptions.CustomOptionsListBox(i - 1).AddItem (itemname)
        
        'if this is the first or the currently config'd choice, make it appear as the current value for the listbox
        If j = 1 Or GetFromINI(GetCurrentGame, "ListBox" & i & "cmdline" & j, GetGameListFile) = GetFromINI(GetCurrentGame, "Option" & i, GetConfigFile) Then frmOptions.CustomOptionsListBox(i - 1).ListIndex = j - 1
    Next j
    frmOptions.CustomOptionsLabel(i - 1).Caption = tmpstr
    frmOptions.CustomOptionsListBox(i - 1).Visible = True
    frmOptions.CustomOptionsLabel(i - 1).Visible = True
End If
       
Next i



'get game options

'this flag keeps track of whether any DIP options were found to display
havestuff = False

'are we using the daphne defaults?
If Len(GetFromINI(GetCurrentGame, "DIPusedefaults", GetConfigFile)) > 0 Then
    usedefaults = GetBoolFromINI(GetCurrentGame, "DIPusedefaults", GetConfigFile)
Else
    'use defaults by default  :)
    usedefaults = True
End If


If usedefaults Then frmOptions.DIPUseDefaultsCheckBox.Value = 1 Else frmOptions.DIPUseDefaultsCheckBox.Value = 0

    For i = 1 To MaxDIPListBoxes
        frmOptions.DIPlistbox(i - 1).Enabled = Not usedefaults
        frmOptions.DIPlistboxlabel(i - 1).Enabled = Not usedefaults
    Next i
    For i = 1 To MaxDIPCheckBoxes
        frmOptions.DIPcheckbox(i - 1).Enabled = Not usedefaults
    
    Next i



'get the dip listboxes

For i = 1 To MaxDIPListBoxes
tmpstr = GetFromINI(GetCurrentGame, "DIPListBox" & i & "Title", GetGameListFile)
If Len(tmpstr) > 0 Then
    
    havestuff = True

    numchoices = GetIntFromINI(GetCurrentGame, "DIPListBox" & i & "Items", 1, GetGameListFile)
    For j = 1 To numchoices
        itemname = GetFromINI(GetCurrentGame, "DIPListBox" & i & "Item" & j & "Name", GetGameListFile)
        frmOptions.DIPlistbox(i - 1).AddItem (itemname)
        
        'if this is the first or the currently config'd choice, make it appear as the current value for the listbox
        If j = 1 Or j = GetIntFromINI(GetCurrentGame, "DIPListBox" & i & "choice", -1, GetConfigFile) Then frmOptions.DIPlistbox(i - 1).ListIndex = j - 1
    Next j
       
    frmOptions.DIPlistboxlabel(i - 1).Caption = tmpstr
    frmOptions.DIPlistbox(i - 1).Visible = True
    frmOptions.DIPlistboxlabel(i - 1).Visible = True
 
End If
Next i



'now get the DIP checkboxes

For i = 1 To MaxDIPCheckBoxes
tmpstr = GetFromINI(GetCurrentGame, "DIPCheckBox" & i & "Title", GetGameListFile)
If Len(tmpstr) > 0 Then
        havestuff = True
       'if this is the first or the currently config'd choice, make it appear as the current value for the listbox
        'If j = 1 Or GetFromINI(GetCurrentGame, "DIPListBox" & i & "ORvalue" & j, GetGameListFile) =
        
        If GetBoolFromINI(GetCurrentGame, "DIPCheckBox" & i & "checked", GetConfigFile) Then frmOptions.DIPcheckbox(i - 1).Value = 1 Else frmOptions.DIPcheckbox(i - 1).Value = 0
       
    frmOptions.DIPcheckbox(i - 1).Caption = tmpstr
    frmOptions.DIPcheckbox(i - 1).Visible = True
    
End If
Next i

'now get the custom options checkboxes

For i = 1 To MaxCustomCheckBoxes
tmpstr = GetFromINI(GetCurrentGame, "CustomCheckBox" & i & "Title", GetGameListFile)
If Len(tmpstr) > 0 Then
        havestuff = True
       'if this is the first or the currently config'd choice, make it appear as the current value for the listbox
        'If j = 1 Or GetFromINI(GetCurrentGame, "DIPListBox" & i & "ORvalue" & j, GetGameListFile) =
        
        If GetBoolFromINI(GetCurrentGame, "CustomCheckBox" & i & "checked", GetConfigFile) Then frmOptions.CustomOptionsCheckBox(i - 1).Value = 1 Else frmOptions.CustomOptionsCheckBox(i - 1).Value = 0
       
    frmOptions.CustomOptionsCheckBox(i - 1).Caption = tmpstr
    frmOptions.CustomOptionsCheckBox(i - 1).Visible = True
    
End If
Next i


If Not havestuff Then
    frmOptions.DIPUseDefaultsCheckBox.Value = 1
    frmOptions.DIPUseDefaultsCheckBox.Enabled = False
End If


'get player options
'get the list of players
numberofplayers = Val(GetFromINI("LDPlayerList", "NumberOfLDPlayers", GetGameListFile))
For i = 1 To numberofplayers
    itemname = GetFromINI("LDPlayerList", "LDPlayer" & i, GetGameListFile)
    frmOptions.LDPlayerList.AddItem (itemname)
    'load the first choice by default, unless another choice is configured
    If i = 1 Or GetFromINI(itemname, "DriverName", GetGameListFile) = GetFromINI(GetCurrentGame, "LDPlayerType", GetConfigFile) Then
        frmOptions.LDPlayerList.ListIndex = i - 1
    End If
    
Next i
'LD-specific options


'show the configured port, or default to the first choice (port 0 - COM1)
portnum = GetIntFromINI(GetCurrentGame, "LDport", -1, GetConfigFile)
If portnum > 0 Then frmOptions.LDportList.ListIndex = portnum Else frmOptions.LDportList.ListIndex = 0
        
'show the configured baud rate, or default to 9600
baudrate = GetIntFromINI(GetCurrentGame, "LDbaudrate", -1, GetConfigFile)
If baudrate > 0 Then frmOptions.LDbaudList.Text = baudrate Else frmOptions.LDbaudList.ListIndex = 3

frmOptions.MPEGFileTextBox.Text = GetFromINI(GetCurrentGame, "MPEGInfoFile", GetConfigFile)
    
If GetBoolFromINI(GetCurrentGame, "BlankSkips", GetConfigFile) Then frmOptions.BlankSkipsCheck.Value = 1
If GetBoolFromINI(GetCurrentGame, "BlankSearches", GetConfigFile) Then frmOptions.BlankSearchesCheck.Value = 1
If GetBoolFromINI(GetCurrentGame, "Blend", GetConfigFile) Then frmOptions.BlendCheck.Value = 1
If GetBoolFromINI(GetCurrentGame, "Scanlines", GetConfigFile) Then frmOptions.ScanlinesCheck.Value = 1
If GetBoolFromINI(GetCurrentGame, "SeekTest", GetConfigFile) Then frmOptions.SeekTest.Value = 1

frmOptions.SeekDelay = GetBoolFromINI(GetCurrentGame, "SeekDelayLDP", GetConfigFile)

If GetBoolFromINI(GetCurrentGame, "SeekDelay", GetConfigFile) Then
    frmOptions.SeekDelayCheck.Value = 1
Else
    frmOptions.SeekDelayCheck.Value = 0
End If

'get list of seek delay parameters
numberofparameters = Val(GetFromINI("SeekDelayParameters", "NumberOfParameters", GetGameListFile))
For i = 1 To numberofparameters
    itemname = GetFromINI("SeekDelayParameters", "Parameter" & i, GetGameListFile)
    frmOptions.SeekDelay.AddItem (itemname)
    'load the first choice by default, unless another choice is configured
    If i = 1 Or itemname = GetFromINI(GetCurrentGame, "SeekDelayLDP", GetConfigFile) Then
        frmOptions.SeekDelay.ListIndex = i - 1
    End If
Next i

'get advanced options

frmOptions.ScreenSizeListBox.ListIndex = GetIntFromINI(GetCurrentGame, "ScreenSize", 0, GetConfigFile)

If GetBoolFromINI(GetCurrentGame, "DisableSound", GetConfigFile) Then frmOptions.SoundCheck.Value = 1
If GetBoolFromINI(GetCurrentGame, "DisableHardwareAcceleration", GetConfigFile) Then frmOptions.YUVCheck.Value = 1
If GetBoolFromINI(GetCurrentGame, "SetSoundBuffer", GetConfigFile) Then frmOptions.SoundBufCheck.Value = 1
If GetBoolFromINI(GetCurrentGame, "SetLatency", GetConfigFile) Then frmOptions.LatencyCheck.Value = 1
If GetBoolFromINI(GetCurrentGame, "DisplayCmdLine", GetConfigFile) Then frmOptions.CmdLineCheck.Value = 1
If GetBoolFromINI(GetCurrentGame, "DisableStats", GetConfigFile) Then frmOptions.StatsCheck.Value = 1
If GetBoolFromINI(GetCurrentGame, "DisableCRC", GetConfigFile) Then frmOptions.DisableCRCCheck.Value = 1

Dim tmpval As Integer
tmpval = GetIntFromINI(GetCurrentGame, "SoundBufferValue", 2048, GetConfigFile)
If tmpval < 256 Or tmpval > 8192 Then tmpval = 2048
Select Case tmpval
Case 256
    frmOptions.SoundBufSlider.Value = 1
Case 512
    frmOptions.SoundBufSlider.Value = 2
Case 1024
    frmOptions.SoundBufSlider.Value = 3
Case 2048
    frmOptions.SoundBufSlider.Value = 4
Case 4096
    frmOptions.SoundBufSlider.Value = 5
Case 8192
    frmOptions.SoundBufSlider.Value = 6
Case Else
    frmOptions.SoundBufSlider.Value = 4
    tmpval = 2048
End Select
frmOptions.SoundBufSlider.Tag = tmpval

frmOptions.LatencySlider.Value = GetIntFromINI(GetCurrentGame, "LatencyValue", 0, GetConfigFile)

frmOptions.CustomSettingsTextbox = GetFromINI(GetCurrentGame, "CustomSettings", GetConfigFile)

'disable user input on the main form
MainForm.Enabled = False

frmOptions.Caption = "Configure " & GetCurrentGame
frmOptions.Show

End Sub



'
'UpdateConfig()
'saves new settings to prefs INI
'
Sub UpdateConfig()


'general tab
If frmOptions.FullscreenCheckbox.Value = 1 Then
    SaveToINI GetCurrentGame, "Fullscreen", "Yes", GetConfigFile
Else
    SaveToINI GetCurrentGame, "Fullscreen", "No", GetConfigFile
End If

For i = 1 To MaxCustomOptions
   If frmOptions.CustomOptionsListBox(i - 1).Visible = True Then
        SaveToINI GetCurrentGame, "Option" & i, GetFromINI(GetCurrentGame, "ListBox" & i & "cmdline" & frmOptions.CustomOptionsListBox(i - 1).ListIndex + 1, GetGameListFile), GetConfigFile
   End If
Next i

'game tab
If frmOptions.DIPUseDefaultsCheckBox.Value = 1 Then
    SaveToINI GetCurrentGame, "DIPUseDefaults", "Yes", GetConfigFile
Else
    SaveToINI GetCurrentGame, "DIPUseDefaults", "No", GetConfigFile
End If
For i = 1 To MaxDIPListBoxes
   If frmOptions.DIPlistbox(i - 1).Visible = True Then
        SaveToINI GetCurrentGame, "DIPListBox" & i & "choice", frmOptions.DIPlistbox(i - 1).ListIndex + 1, GetConfigFile
   End If
Next i
For i = 1 To MaxDIPCheckBoxes
   If frmOptions.DIPcheckbox(i - 1).Visible = True Then
        If frmOptions.DIPcheckbox(i - 1).Value = 1 Then
            Check$ = "Yes"
        Else
            Check$ = "No"
        End If
        
        SaveToINI GetCurrentGame, "DIPCheckBox" & i & "checked", Check$, GetConfigFile
        
   End If
Next i
For i = 1 To MaxCustomCheckBoxes
   If frmOptions.CustomOptionsCheckBox(i - 1).Visible = True Then
        If frmOptions.CustomOptionsCheckBox(i - 1).Value = 1 Then
            Check$ = "Yes"
        Else
            Check$ = "No"
        End If
        
        SaveToINI GetCurrentGame, "CustomCheckBox" & i & "checked", Check$, GetConfigFile
        
   End If
Next i

'laserdisc tab
    SaveToINI GetCurrentGame, "LDPlayerType", GetFromINI(frmOptions.LDPlayerList.Text, "DriverName", GetGameListFile), GetConfigFile
    
    SaveToINI GetCurrentGame, "MPEGInfoFile", frmOptions.MPEGFileTextBox, GetConfigFile
    
    If frmOptions.BlankSkipsCheck.Value = 1 Then
        SaveToINI GetCurrentGame, "BlankSkips", "Yes", GetConfigFile
    Else
        SaveToINI GetCurrentGame, "BlankSkips", "No", GetConfigFile
    End If
    If frmOptions.BlankSearchesCheck.Value = 1 Then
        SaveToINI GetCurrentGame, "BlankSearches", "Yes", GetConfigFile
    Else
        SaveToINI GetCurrentGame, "BlankSearches", "No", GetConfigFile
    End If
    If frmOptions.BlendCheck.Value = 1 Then
        SaveToINI GetCurrentGame, "Blend", "Yes", GetConfigFile
    Else
        SaveToINI GetCurrentGame, "Blend", "No", GetConfigFile
    End If
    If frmOptions.ScanlinesCheck.Value = 1 Then
        SaveToINI GetCurrentGame, "Scanlines", "Yes", GetConfigFile
    Else
        SaveToINI GetCurrentGame, "Scanlines", "No", GetConfigFile
    End If
    If frmOptions.SeekTest.Value = 1 Then
        SaveToINI GetCurrentGame, "SeekTest", "Yes", GetConfigFile
    Else
        SaveToINI GetCurrentGame, "SeekTest", "No", GetConfigFile
    End If
    
    If frmOptions.SeekDelayCheck.Value = 1 Then
        SaveToINI GetCurrentGame, "SeekDelay", "Yes", GetConfigFile
    Else
        SaveToINI GetCurrentGame, "SeekDelay", "No", GetConfigFile
    End If
    
    SaveToINI GetCurrentGame, "SeekDelayLDP", frmOptions.SeekDelay, GetConfigFile
       
    SaveToINI GetCurrentGame, "LDport", frmOptions.LDportList.ListIndex, GetConfigFile
    SaveToINI GetCurrentGame, "LDbaudrate", frmOptions.LDbaudList.Text, GetConfigFile
        
'advanced tab
SaveToINI GetCurrentGame, "ScreenSize", frmOptions.ScreenSizeListBox.ListIndex, GetConfigFile

If frmOptions.SoundCheck.Value = 1 Then
    SaveToINI GetCurrentGame, "DisableSound", "Yes", GetConfigFile
Else
    SaveToINI GetCurrentGame, "DisableSound", "No", GetConfigFile
End If

If frmOptions.YUVCheck.Value = 1 Then
    SaveToINI GetCurrentGame, "DisableHardwareAcceleration", "Yes", GetConfigFile
Else
    SaveToINI GetCurrentGame, "DisableHardwareAcceleration", "No", GetConfigFile
End If

If frmOptions.SoundBufCheck.Value = 1 Then
    SaveToINI GetCurrentGame, "SetSoundBuffer", "Yes", GetConfigFile
Else
    SaveToINI GetCurrentGame, "SetSoundBuffer", "No", GetConfigFile
End If

If frmOptions.LatencyCheck.Value = 1 Then
    SaveToINI GetCurrentGame, "SetLatency", "Yes", GetConfigFile
Else
    SaveToINI GetCurrentGame, "SetLatency", "No", GetConfigFile
End If

If frmOptions.CmdLineCheck.Value = 1 Then
    SaveToINI GetCurrentGame, "DisplayCmdLine", "Yes", GetConfigFile
Else
    SaveToINI GetCurrentGame, "DisplayCmdLine", "No", GetConfigFile
End If

If frmOptions.StatsCheck.Value = 1 Then
    SaveToINI GetCurrentGame, "DisableStats", "Yes", GetConfigFile
Else
    SaveToINI GetCurrentGame, "DisableStats", "No", GetConfigFile
End If

If frmOptions.DisableCRCCheck.Value = 1 Then
    SaveToINI GetCurrentGame, "DisableCRC", "Yes", GetConfigFile
Else
    SaveToINI GetCurrentGame, "DisableCRC", "No", GetConfigFile
End If

SaveToINI GetCurrentGame, "SoundBufferValue", frmOptions.SoundBufSlider.Tag, GetConfigFile
SaveToINI GetCurrentGame, "LatencyValue", frmOptions.LatencySlider.Value, GetConfigFile
SaveToINI GetCurrentGame, "CustomSettings", frmOptions.CustomSettingsTextbox, GetConfigFile

End Sub



'
'GetCmdLine()
'parse the prefs INI to get the user options for the selected game
'
Function GetCmdLine(GameName As String)
Dim CmdLine As String
Dim tmpstr As String
Dim orvalarray(MaxDIPBanks)
Dim ShowNoLDPWarning As Boolean
Dim SeekTestToggle As Boolean

ShowNoLDPWarning = False
SeekTestToggle = False
 
'first check if we are doing a VLDP Seek Test
If GetBoolFromINI(GameName, "SeekTest", GetConfigFile) And UCase(GetFromINI(GameName, "LDPlayerType", GetConfigFile)) = "VLDP" Then
    SeekTestToggle = True
    
    tmpstr = GetFromINI("MPEG Seek Test", "SeekTestCommand", GetGameListFile)
    CmdLine = CmdLine & tmpstr
    tmpstr = GetFromINI(GameName, "Option1", GetConfigFile)
    
    'check for exceptions (non-standard laserdisc types)
    Select Case tmpstr
    Case "-pal_dl":
        tmpstr = GetFromINI("MPEG Seek Test", "Philips 1983 PAL DL", GetGameListFile)
    Case "-pal_dl_sc":
        tmpstr = GetFromINI("MPEG Seek Test", "Dragon's Lair PAL (Software Corner)", GetGameListFile)
    Case Else
        tmpstr = GetFromINI("MPEG Seek Test", GameName, GetGameListFile)
    End Select
          
    If tmpstr = "none" Then
        MsgBox "Seektest not available for this game.", vbOKOnly + vbExclamation, "Unable to Run Seek Test"
            GetCmdLine = ""
        Exit Function
    End If
    CmdLine = CmdLine & " " & tmpstr & " "
    tmpstr2 = GetFromINI(GameName, "LDPlayerType", GetConfigFile)
Else
    'we are not doing a seektest so get the game and laserdisc driver names
    tmpstr = GetFromINI(GameName, "DriverName", GetGameListFile)
    CmdLine = CmdLine & tmpstr
    
    tmpstr2 = GetFromINI(GameName, "LDPlayerType", GetConfigFile)
    If tmpstr2 = "" Then tmpstr2 = "noldp" 'default to no ldp if game is not configured
    
    CmdLine = CmdLine & " " & tmpstr2 & " "  'add the player to the cmdline
End If

If tmpstr2 = "noldp" Then ShowNoLDPWarning = True  ' enable a warning for the newbies

'this is a little cheesy, it *should* use the playertype info in the config file
If UCase(tmpstr2) = "VLDP" Then
    tmpstr = GetFromINI(GameName, "MPEGInfoFile", GetConfigFile)
    If tmpstr = "" Then
        MsgBox "No MPEG Info File Specified - Please Reconfigure.", vbOKOnly + vbExclamation, "Unable To Start Game"
            GetCmdLine = ""
        Exit Function
    End If

CmdLine = CmdLine & "-framefile """ & tmpstr & """ "

Else
    If Not (UCase(tmpstr2) = "NOLDP") Then
        'must be a serial player, add the serial options
        portnum = GetIntFromINI(GameName, "LDport", -1, GetConfigFile)
        If portnum > 0 Then CmdLine = CmdLine & "-port " & portnum & " "
        
        baudrate = GetIntFromINI(GameName, "LDbaudrate", -1, GetConfigFile)
        If baudrate > 0 Then CmdLine = CmdLine & "-baud " & baudrate & " "
    End If
End If

'get the general options
If GetBoolFromINI(GameName, "Fullscreen", GetConfigFile) Then CmdLine = CmdLine & "-fullscreen "

If GetBoolFromINI(GameName, "BlankSkips", GetConfigFile) Then CmdLine = CmdLine & "-blank_skips "
If GetBoolFromINI(GameName, "BlankSearches", GetConfigFile) Then CmdLine = CmdLine & "-blank_searches "
If GetBoolFromINI(GameName, "Blend", GetConfigFile) Then CmdLine = CmdLine & "-blend "
If GetBoolFromINI(GameName, "Scanlines", GetConfigFile) Then CmdLine = CmdLine & "-scanlines "
For i = 1 To MaxCustomOptions
    tmpstr = GetFromINI(GameName, "Option" & i, GetConfigFile)
    If Len(tmpstr) > 0 Then CmdLine = CmdLine & tmpstr & " "
Next i

'seek delay options
SeekDelayLDP = GetFromINI(GameName, "SeekDelayLDP", GetConfigFile)
If GetBoolFromINI(GameName, "SeekDelay", GetConfigFile) Then
    If SeekDelayLDP = "Custom" Then
        tmpstr = GetFromINI(GameName, "SeekDelayCustom", GetConfigFile)
    Else
        numberofparameters = Val(GetFromINI("SeekDelayParameters", "NumberOfParameters", GetGameListFile))
        For i = 1 To numberofparameters
        tmpstr2 = GetFromINI("SeekDelayParameters", "Parameter" & i, GetGameListFile)
        If tmpstr2 = SeekDelayLDP Then tmpstr = GetFromINI("SeekDelayParameters", "Parameter" & i & "Delay", GetGameListFile)
        Next i
    End If
CmdLine = CmdLine & tmpstr & " "
End If
   
For i = 1 To MaxCustomCheckBoxes
            
    If Len(GetFromINI(GameName, "Customcheckbox" & i & "title", GetGameListFile)) > 0 Then
                               
            If GetBoolFromINI(GameName, "CustomCheckBox" & i & "checked", GetConfigFile) Then
                tmpstr = GetFromINI(GameName, "CustomCheckBox" & i & "onvalue", GetGameListFile)
            Else
                tmpstr = GetFromINI(GameName, "CustomCheckBox" & i & "offvalue", GetGameListFile)
            End If
                      
    CmdLine = CmdLine & tmpstr & " "
                      
    End If
        
Next i
   
'get the game options
If GetBoolFromINI(GameName, "DIPusedefaults", GetConfigFile) = False Then

    maxbanks = GetIntFromINI(GameName, "DIPbanks", 0, GetGameListFile)
    If maxbanks > 0 Then

        'initialize the dip switch bank values
        For i = 1 To maxbanks
            orvalarray(maxbanks) = 0
        Next i
        
        'add the values for the ListBox options
        For i = 1 To MaxDIPListBoxes
            choice = GetIntFromINI(GameName, "DIPlistbox" & i & "choice", -1, GetConfigFile)
            If choice > -1 Then
                For j = 1 To maxbanks
                    orval = GetIntFromINI(GameName, "DIPlistbox" & i & "item" & choice & "bank" & j - 1 & "orvalue", -1, GetGameListFile)
                    If orval > -1 Then orvalarray(j - 1) = orvalarray(j - 1) Or orval
                Next j
            End If
        Next i
        
        'add the values for the checkbox options
        For i = 1 To MaxDIPCheckBoxes
            
            If Len(GetFromINI(GameName, "DIPcheckbox" & i & "title", GetGameListFile)) > 0 Then
            
                banknumber = GetIntFromINI(GameName, "DIPcheckbox" & i & "bank", -1, GetGameListFile)
                        
                If banknumber > -1 Then
                    If GetBoolFromINI(GameName, "DIPcheckbox" & i & "checked", GetConfigFile) Then
                        orval = GetIntFromINI(GameName, "DIPcheckbox" & i & "onvalue", -1, GetGameListFile)
                    Else
                        orval = GetIntFromINI(GameName, "DIPcheckbox" & i & "offvalue", -1, GetGameListFile)
                    End If
                If orval > -1 Then orvalarray(banknumber) = orvalarray(banknumber) Or orval
            End If
        
            End If
        
        Next i
        
    'now add the banks to the command line
        If SeekTestToggle = False Then
    
            For i = 1 To maxbanks
                CmdLine = CmdLine & "-bank " & i - 1 & " " & ToBinary(Str$(orvalarray(i - 1))) & " "
            Next i
    
        End If
        
    End If
    
End If

'(laserdisc options already handled above)

'get the advanced options

screensize = GetIntFromINI(GameName, "ScreenSize", -1, GetConfigFile)

Select Case screensize
Case 0
    ' default, do nothing
    screenmode = ""
Case 1
    screenmode = "-x 640 -y 480"
Case 2
    screenmode = "-x 720 -y 480"
Case 3
    screenmode = "-x 800 -y 600"
Case 4
    screenmode = "-x 1024 -y 768"
Case 5
    screenmode = "-x 1152 -y 864"
Case 6
    screenmode = "-x 1280 -y 1024"
Case 7
    screenmode = "-x 1600 -y 1200"
End Select

CmdLine = CmdLine & screenmode & " "

If GetBoolFromINI(GameName, "DisableSound", GetConfigFile) Then CmdLine = CmdLine & "-nosound "
If GetBoolFromINI(GameName, "DisableHardwareAcceleration", GetConfigFile) Then CmdLine = CmdLine & "-nohwaccel "
If GetBoolFromINI(GameName, "SetSoundBuffer", GetConfigFile) Then
    Dim bufsize As Integer
    bufsize = GetIntFromINI(GameName, "SoundBufferValue", -1, GetConfigFile)
    If bufsize > -1 Then CmdLine = CmdLine & "-sound_buffer " & bufsize & " "
End If
If GetBoolFromINI(GameName, "SetLatency", GetConfigFile) Then
    Dim latency As Integer
    latency = GetIntFromINI(GameName, "LatencyValue", -1, GetConfigFile)
    If latency > -1 Then CmdLine = CmdLine & "-latency " & latency & " "
End If

If GetBoolFromINI(GameName, "DisableStats", GetConfigFile) Then CmdLine = CmdLine & "-noserversend "
If GetBoolFromINI(GameName, "DisableCRC", GetConfigFile) Then CmdLine = CmdLine & "-nocrc "

CmdLine = CmdLine & GetFromINI(GameName, "CustomSettings", GetConfigFile)

'display the cmdline if req'd, allow user to cancel (clears cmd line)
If GetBoolFromINI(GameName, "DisplayCmdLine", GetConfigFile) Then
    'If MsgBox(App.Path & "\daphne.exe " & CmdLine, vbOKCancel, "Launching Daphne") = vbCancel Then
    '    GetCmdLine = ""
    '    Exit Function
    'End If
    CommandLineDlg.Text1.Text = App.Path & "\daphne.exe " & CmdLine
    CommandLineDlg.Show vbModal, MainForm
    
    If CommandLineDlg.Tag = vbCancel Then
        CmdLine = ""  ' check if the user wanted to stop here
        Exit Function
    End If
End If

If ShowNoLDPWarning And GetBoolFromINI("General", "SkipNoldpWarning", GetGameListFile) = False Then
    If MsgBox("WARNING:  You are starting Daphne with no laserdisc player or MPEG file configured.  " & vbCr & _
      "This mode is only useful for testing.  You will NOT be able to play the game properly." & vbCr & vbCr & _
      "Are you sure you want to continue?", vbYesNo + vbQuestion, "Launching Daphne") = vbNo Then
        GetCmdLine = ""
        Exit Function
    End If
End If


GetCmdLine = CmdLine

End Function

Function ToBinary(num As String) As String
'takes a decimal # (0-255) as a string, returns a binary number as a string

Dim tmpval, chkval As Integer
Dim tmpstr As String

tmpstr = ""
tmpval = Val(num)

For i = 7 To 0 Step -1
    chkval = tmpval And (2 ^ i)
    If chkval > 0 Then
       tmpstr = tmpstr & "1"
    Else
        tmpstr = tmpstr & "0"
    End If
Next i

ToBinary = tmpstr

End Function

