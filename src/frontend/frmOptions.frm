VERSION 5.00
Object = "{831FDD16-0C5C-11D2-A9FC-0000F8754DA1}#2.0#0"; "Mscomctl.ocx"
Object = "{F9043C88-F6F2-101A-A3C9-08002B2F49FB}#1.2#0"; "Comdlg32.ocx"
Begin VB.Form frmOptions 
   BorderStyle     =   3  'Fixed Dialog
   Caption         =   "Configure Game"
   ClientHeight    =   5430
   ClientLeft      =   4440
   ClientTop       =   5070
   ClientWidth     =   6120
   Icon            =   "frmOptions.frx":0000
   KeyPreview      =   -1  'True
   LinkTopic       =   "Form1"
   MaxButton       =   0   'False
   MinButton       =   0   'False
   ScaleHeight     =   5430
   ScaleWidth      =   6120
   StartUpPosition =   1  'CenterOwner
   Begin VB.CommandButton Command1 
      Caption         =   "Config B&uttons"
      Height          =   375
      Left            =   1320
      TabIndex        =   68
      Top             =   4800
      Width           =   1215
   End
   Begin VB.CommandButton EditKeysButton 
      Caption         =   "Config &Keys"
      Height          =   375
      Left            =   120
      TabIndex        =   31
      Top             =   4800
      Width           =   1095
   End
   Begin VB.CommandButton cmdOK 
      Caption         =   "&OK"
      Height          =   375
      Left            =   2880
      TabIndex        =   26
      Top             =   4800
      Width           =   975
   End
   Begin MSComDlg.CommonDialog FileDialog 
      Left            =   120
      Top             =   4800
      _ExtentX        =   847
      _ExtentY        =   847
      _Version        =   393216
   End
   Begin VB.CommandButton cmdApply 
      Caption         =   "Apply"
      Height          =   375
      Left            =   5040
      TabIndex        =   30
      Top             =   4800
      Width           =   975
   End
   Begin VB.CommandButton cmdCancel 
      Cancel          =   -1  'True
      Caption         =   "Cancel"
      Height          =   375
      Left            =   3960
      TabIndex        =   28
      Top             =   4800
      Width           =   975
   End
   Begin VB.PictureBox picOptions 
      BorderStyle     =   0  'None
      Height          =   3900
      Index           =   0
      Left            =   240
      ScaleHeight     =   3900
      ScaleWidth      =   5685
      TabIndex        =   2
      TabStop         =   0   'False
      Top             =   600
      Width           =   5685
      Begin VB.CheckBox CustomOptionsCheckBox 
         Caption         =   "Check2"
         Height          =   495
         Index           =   1
         Left            =   3000
         TabIndex        =   81
         Top             =   0
         Visible         =   0   'False
         Width           =   1215
      End
      Begin VB.CheckBox CustomOptionsCheckBox 
         Caption         =   "Check1"
         Height          =   495
         Index           =   0
         Left            =   3000
         TabIndex        =   80
         Top             =   360
         Visible         =   0   'False
         Width           =   1215
      End
      Begin VB.ComboBox CustomOptionsListBox 
         Height          =   315
         Index           =   5
         Left            =   2040
         TabIndex        =   78
         Text            =   "Combo1"
         Top             =   3360
         Visible         =   0   'False
         Width           =   3495
      End
      Begin VB.ComboBox CustomOptionsListBox 
         Height          =   315
         Index           =   2
         Left            =   2040
         TabIndex        =   32
         Text            =   "Combo1"
         Top             =   1920
         Visible         =   0   'False
         Width           =   3495
      End
      Begin VB.ComboBox CustomOptionsListBox 
         Height          =   315
         Index           =   1
         Left            =   2040
         TabIndex        =   29
         Text            =   "Combo1"
         Top             =   1440
         Visible         =   0   'False
         Width           =   3495
      End
      Begin VB.ComboBox CustomOptionsListBox 
         Height          =   315
         Index           =   0
         Left            =   2040
         TabIndex        =   27
         Text            =   "Combo1"
         Top             =   960
         Visible         =   0   'False
         Width           =   3495
      End
      Begin VB.CheckBox FullscreenCheckbox 
         Caption         =   "Run &Fullscreen"
         Height          =   255
         Left            =   480
         TabIndex        =   23
         Top             =   360
         Width           =   1935
      End
      Begin VB.ComboBox CustomOptionsListBox 
         Height          =   315
         Index           =   3
         Left            =   2040
         TabIndex        =   33
         Text            =   "Combo1"
         Top             =   2400
         Visible         =   0   'False
         Width           =   3495
      End
      Begin VB.ComboBox CustomOptionsListBox 
         Height          =   315
         Index           =   4
         Left            =   2040
         TabIndex        =   65
         Text            =   "Combo1"
         Top             =   2880
         Visible         =   0   'False
         Width           =   3495
      End
      Begin VB.Label CustomOptionsLabel 
         Alignment       =   1  'Right Justify
         Caption         =   "CustomOption"
         Height          =   495
         Index           =   5
         Left            =   120
         TabIndex        =   79
         Top             =   3360
         Visible         =   0   'False
         Width           =   1815
      End
      Begin VB.Label CustomOptionsLabel 
         Alignment       =   1  'Right Justify
         Caption         =   "CustomOption"
         Height          =   495
         Index           =   4
         Left            =   120
         TabIndex        =   66
         Top             =   2880
         Visible         =   0   'False
         Width           =   1815
      End
      Begin VB.Label CustomOptionsLabel 
         Alignment       =   1  'Right Justify
         Caption         =   "CustomOption"
         Height          =   495
         Index           =   3
         Left            =   120
         TabIndex        =   37
         Top             =   2400
         Visible         =   0   'False
         Width           =   1815
      End
      Begin VB.Label CustomOptionsLabel 
         Alignment       =   1  'Right Justify
         Caption         =   "CustomOption"
         Height          =   495
         Index           =   2
         Left            =   120
         TabIndex        =   36
         Top             =   1920
         Visible         =   0   'False
         Width           =   1815
      End
      Begin VB.Label CustomOptionsLabel 
         Alignment       =   1  'Right Justify
         Caption         =   "CustomOption"
         Height          =   495
         Index           =   0
         Left            =   120
         TabIndex        =   34
         Top             =   960
         Visible         =   0   'False
         Width           =   1815
      End
      Begin VB.Label CustomOptionsLabel 
         Alignment       =   1  'Right Justify
         Caption         =   "CustomOption"
         Height          =   495
         Index           =   1
         Left            =   120
         TabIndex        =   35
         Top             =   1440
         Visible         =   0   'False
         Width           =   1815
      End
   End
   Begin VB.PictureBox picOptions 
      BorderStyle     =   0  'None
      Height          =   3660
      Index           =   2
      Left            =   240
      ScaleHeight     =   3660
      ScaleWidth      =   5685
      TabIndex        =   4
      TabStop         =   0   'False
      Top             =   600
      Width           =   5685
      Begin VB.ComboBox LDPlayerList 
         Height          =   315
         Left            =   120
         TabIndex        =   6
         Text            =   "Player List"
         Top             =   240
         Width           =   3615
      End
      Begin VB.CheckBox SeekTest 
         Caption         =   "MPEG Seek &Test"
         Height          =   375
         Left            =   3960
         TabIndex        =   67
         Top             =   240
         Width           =   1695
      End
      Begin VB.Frame VLDPoptionsFrame 
         Caption         =   "Virtual Laserdisc Player Options"
         BeginProperty Font 
            Name            =   "MS Sans Serif"
            Size            =   8.25
            Charset         =   0
            Weight          =   700
            Underline       =   0   'False
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         Height          =   2895
         Left            =   120
         TabIndex        =   7
         Top             =   720
         Width           =   5535
         Begin VB.CheckBox ScanlinesCheck 
            Caption         =   "Simulated S&canlines"
            Height          =   255
            Left            =   360
            TabIndex        =   77
            Top             =   2520
            Width           =   2055
         End
         Begin VB.CheckBox BlendCheck 
            Caption         =   "Realtime &De-Interlace"
            Height          =   255
            Left            =   360
            TabIndex        =   76
            Top             =   2280
            Width           =   2055
         End
         Begin VB.CheckBox BlankSearchesCheck 
            Caption         =   "Blank on &searches"
            Height          =   255
            Left            =   360
            TabIndex        =   75
            Top             =   1680
            Width           =   1815
         End
         Begin VB.ComboBox SeekDelay 
            Height          =   315
            Left            =   2520
            TabIndex        =   69
            Text            =   "Player List"
            Top             =   2400
            Visible         =   0   'False
            Width           =   2655
         End
         Begin VB.CheckBox SeekDelayCheck 
            Caption         =   "Seek Delay"
            Height          =   255
            Left            =   2520
            TabIndex        =   63
            Top             =   1680
            Width           =   1335
         End
         Begin VB.CheckBox BlankSkipsCheck 
            Caption         =   "Blank on s&kips"
            Height          =   255
            Left            =   360
            TabIndex        =   62
            Top             =   1920
            Width           =   1575
         End
         Begin VB.Frame SelectFrameFileFrame 
            Caption         =   "Frame File Location"
            Height          =   1305
            Left            =   120
            TabIndex        =   71
            Top             =   240
            Width           =   5265
            Begin VB.CommandButton EditButton 
               Caption         =   "&Edit"
               Height          =   375
               Left            =   1410
               TabIndex        =   74
               Top             =   720
               Width           =   885
            End
            Begin VB.CommandButton FileBrowseButton 
               Caption         =   "&Browse"
               Height          =   360
               Left            =   345
               TabIndex        =   73
               Top             =   720
               Width           =   885
            End
            Begin VB.TextBox MPEGFileTextBox 
               Height          =   285
               Left            =   240
               TabIndex        =   72
               Text            =   "Frame File"
               Top             =   300
               Width           =   4815
            End
         End
         Begin VB.Label SeekDelayLabel 
            Caption         =   "LDP Simulated"
            Height          =   255
            Left            =   2520
            TabIndex        =   70
            Top             =   2160
            Visible         =   0   'False
            Width           =   1455
         End
      End
      Begin VB.Frame SerialLDoptionsFrame 
         Caption         =   "LD Player Options"
         Height          =   1785
         Left            =   480
         TabIndex        =   5
         Top             =   1320
         Width           =   4815
         Begin VB.ComboBox LDbaudList 
            Height          =   315
            ItemData        =   "frmOptions.frx":08CA
            Left            =   1200
            List            =   "frmOptions.frx":08DA
            TabIndex        =   9
            Text            =   "9600"
            Top             =   960
            Width           =   1335
         End
         Begin VB.ComboBox LDportList 
            Height          =   315
            ItemData        =   "frmOptions.frx":08F6
            Left            =   1200
            List            =   "frmOptions.frx":0912
            TabIndex        =   8
            Text            =   "COM1"
            Top             =   480
            Width           =   1335
         End
         Begin VB.Label Label8 
            Caption         =   "Baud Rate"
            Height          =   255
            Left            =   240
            TabIndex        =   11
            Top             =   960
            Width           =   855
         End
         Begin VB.Label Label7 
            Caption         =   "Serial Port"
            Height          =   255
            Left            =   360
            TabIndex        =   10
            Top             =   480
            Width           =   735
         End
      End
      Begin VB.Label Label13 
         Caption         =   "Player Type:"
         Height          =   375
         Left            =   120
         TabIndex        =   39
         Top             =   0
         Width           =   2295
      End
   End
   Begin VB.PictureBox picOptions 
      BorderStyle     =   0  'None
      Height          =   3660
      Index           =   1
      Left            =   240
      ScaleHeight     =   3660
      ScaleWidth      =   5595
      TabIndex        =   3
      TabStop         =   0   'False
      Top             =   480
      Width           =   5595
      Begin VB.CheckBox DIPcheckbox 
         Caption         =   "DIPcheckbox"
         Height          =   255
         Index           =   5
         Left            =   240
         TabIndex        =   60
         Top             =   960
         Visible         =   0   'False
         Width           =   2415
      End
      Begin VB.CheckBox DIPcheckbox 
         Caption         =   "DIPcheckbox"
         Height          =   255
         Index           =   1
         Left            =   2760
         TabIndex        =   54
         Top             =   720
         Visible         =   0   'False
         Width           =   2415
      End
      Begin VB.CheckBox DIPcheckbox 
         Caption         =   "DIPcheckbox"
         Height          =   255
         Index           =   4
         Left            =   240
         TabIndex        =   59
         Top             =   720
         Visible         =   0   'False
         Width           =   2415
      End
      Begin VB.CheckBox DIPcheckbox 
         Caption         =   "DIPcheckbox"
         Height          =   255
         Index           =   3
         Left            =   240
         TabIndex        =   58
         Top             =   480
         Visible         =   0   'False
         Width           =   2415
      End
      Begin VB.CheckBox DIPcheckbox 
         Caption         =   "DIPcheckbox"
         Height          =   255
         Index           =   2
         Left            =   2760
         TabIndex        =   57
         Top             =   960
         Visible         =   0   'False
         Width           =   2415
      End
      Begin VB.ComboBox DIPlistbox 
         Height          =   315
         Index           =   6
         ItemData        =   "frmOptions.frx":0946
         Left            =   2760
         List            =   "frmOptions.frx":0948
         TabIndex        =   55
         Text            =   "DIPlistbox"
         Top             =   3360
         Visible         =   0   'False
         Width           =   2775
      End
      Begin VB.ComboBox DIPlistbox 
         Height          =   315
         Index           =   4
         ItemData        =   "frmOptions.frx":094A
         Left            =   2760
         List            =   "frmOptions.frx":094C
         TabIndex        =   45
         Text            =   "DIPlistbox"
         Top             =   2640
         Visible         =   0   'False
         Width           =   2775
      End
      Begin VB.ComboBox DIPlistbox 
         Height          =   315
         Index           =   3
         ItemData        =   "frmOptions.frx":094E
         Left            =   2760
         List            =   "frmOptions.frx":0950
         TabIndex        =   44
         Text            =   "DIPlistbox"
         Top             =   2280
         Visible         =   0   'False
         Width           =   2775
      End
      Begin VB.ComboBox DIPlistbox 
         Height          =   315
         Index           =   2
         ItemData        =   "frmOptions.frx":0952
         Left            =   2760
         List            =   "frmOptions.frx":0954
         TabIndex        =   43
         Text            =   "DIPlistbox"
         Top             =   1920
         Visible         =   0   'False
         Width           =   2775
      End
      Begin VB.ComboBox DIPlistbox 
         Height          =   315
         Index           =   1
         ItemData        =   "frmOptions.frx":0956
         Left            =   2760
         List            =   "frmOptions.frx":0958
         TabIndex        =   42
         Text            =   "DIPlistbox"
         Top             =   1560
         Visible         =   0   'False
         Width           =   2775
      End
      Begin VB.ComboBox DIPlistbox 
         Height          =   315
         Index           =   0
         ItemData        =   "frmOptions.frx":095A
         Left            =   2760
         List            =   "frmOptions.frx":095C
         TabIndex        =   41
         Text            =   "DIPlistbox"
         Top             =   1200
         Visible         =   0   'False
         Width           =   2775
      End
      Begin VB.CheckBox DIPUseDefaultsCheckBox 
         Caption         =   "&Use Factory/Daphne Defaults"
         Height          =   255
         Left            =   120
         TabIndex        =   40
         Top             =   120
         Width           =   2655
      End
      Begin VB.CheckBox DIPcheckbox 
         Caption         =   "DIPcheckbox"
         Height          =   255
         Index           =   6
         Left            =   2760
         TabIndex        =   61
         Top             =   240
         Visible         =   0   'False
         Width           =   2415
      End
      Begin VB.CheckBox DIPcheckbox 
         Caption         =   "DIPcheckbox"
         Height          =   255
         Index           =   0
         Left            =   2760
         TabIndex        =   53
         Top             =   480
         Visible         =   0   'False
         Width           =   2415
      End
      Begin VB.ComboBox DIPlistbox 
         Height          =   315
         Index           =   5
         ItemData        =   "frmOptions.frx":095E
         Left            =   2760
         List            =   "frmOptions.frx":0960
         TabIndex        =   46
         Text            =   "DIPlistbox"
         Top             =   3000
         Visible         =   0   'False
         Width           =   2775
      End
      Begin VB.Label DIPlistboxlabel 
         Alignment       =   1  'Right Justify
         Caption         =   "DIPlistboxlabel"
         Height          =   255
         Index           =   1
         Left            =   0
         TabIndex        =   48
         Top             =   1560
         Visible         =   0   'False
         Width           =   2655
      End
      Begin VB.Label DIPlistboxlabel 
         Alignment       =   1  'Right Justify
         Caption         =   "DIPlistboxlabel"
         Height          =   255
         Index           =   6
         Left            =   0
         TabIndex        =   56
         Top             =   3360
         Visible         =   0   'False
         Width           =   2655
      End
      Begin VB.Label DIPlistboxlabel 
         Alignment       =   1  'Right Justify
         Caption         =   "DIPlistboxlabel"
         Height          =   255
         Index           =   5
         Left            =   0
         TabIndex        =   52
         Top             =   3000
         Visible         =   0   'False
         Width           =   2655
      End
      Begin VB.Label DIPlistboxlabel 
         Alignment       =   1  'Right Justify
         Caption         =   "DIPlistboxlabel"
         Height          =   255
         Index           =   0
         Left            =   0
         TabIndex        =   47
         Top             =   1200
         Visible         =   0   'False
         Width           =   2655
      End
      Begin VB.Label DIPlistboxlabel 
         Alignment       =   1  'Right Justify
         Caption         =   "DIPlistboxlabel"
         Height          =   255
         Index           =   3
         Left            =   0
         TabIndex        =   50
         Top             =   2280
         Visible         =   0   'False
         Width           =   2655
      End
      Begin VB.Label DIPlistboxlabel 
         Alignment       =   1  'Right Justify
         Caption         =   "DIPlistboxlabel"
         Height          =   255
         Index           =   2
         Left            =   0
         TabIndex        =   49
         Top             =   1920
         Visible         =   0   'False
         Width           =   2655
      End
      Begin VB.Label DIPlistboxlabel 
         Alignment       =   1  'Right Justify
         Caption         =   "DIPlistboxlabel"
         Height          =   255
         Index           =   4
         Left            =   0
         TabIndex        =   51
         Top             =   2640
         Visible         =   0   'False
         Width           =   2655
      End
   End
   Begin VB.PictureBox picOptions 
      BorderStyle     =   0  'None
      Height          =   3660
      Index           =   3
      Left            =   240
      ScaleHeight     =   3660
      ScaleWidth      =   5685
      TabIndex        =   12
      TabStop         =   0   'False
      Top             =   480
      Width           =   5685
      Begin VB.CheckBox SoundBufCheck 
         Caption         =   "Sound &Buffer Size"
         Height          =   255
         Left            =   480
         TabIndex        =   17
         Top             =   1320
         Width           =   1815
      End
      Begin VB.CheckBox DisableCRCCheck 
         Caption         =   "Disable &ROM CRC check"
         Height          =   255
         Left            =   480
         TabIndex        =   22
         Top             =   2400
         Width           =   4095
      End
      Begin VB.CheckBox StatsCheck 
         Caption         =   "Don't &report stats"
         Height          =   255
         Left            =   480
         TabIndex        =   21
         Top             =   2040
         Width           =   4095
      End
      Begin VB.ComboBox ScreenSizeListBox 
         Height          =   315
         ItemData        =   "frmOptions.frx":0962
         Left            =   1680
         List            =   "frmOptions.frx":097E
         TabIndex        =   14
         Text            =   "Combo1"
         Top             =   240
         Width           =   1815
      End
      Begin VB.CheckBox CmdLineCheck 
         Caption         =   "Display &command line before launching"
         Height          =   255
         Left            =   480
         TabIndex        =   24
         Top             =   2760
         Width           =   4095
      End
      Begin MSComctlLib.Slider LatencySlider 
         Height          =   255
         Left            =   2280
         TabIndex        =   20
         Top             =   1680
         Width           =   2055
         _ExtentX        =   3625
         _ExtentY        =   450
         _Version        =   393216
         Enabled         =   0   'False
      End
      Begin VB.CheckBox LatencyCheck 
         Caption         =   "LD &latency adjust"
         Height          =   255
         Left            =   480
         TabIndex        =   19
         Top             =   1680
         Width           =   1815
      End
      Begin VB.CheckBox SoundCheck 
         Caption         =   "Disable &Sound"
         Height          =   255
         Left            =   480
         TabIndex        =   15
         Top             =   600
         Width           =   1575
      End
      Begin VB.CheckBox YUVCheck 
         Caption         =   "Disable &hardware acceleration (MPEG only)"
         Height          =   255
         Left            =   480
         TabIndex        =   16
         Top             =   960
         Width           =   4335
      End
      Begin VB.TextBox CustomSettingsTextbox 
         Height          =   285
         Left            =   1560
         TabIndex        =   25
         Top             =   3240
         Width           =   3495
      End
      Begin MSComctlLib.Slider SoundBufSlider 
         Height          =   255
         Left            =   2280
         TabIndex        =   18
         Top             =   1320
         Width           =   2055
         _ExtentX        =   3625
         _ExtentY        =   450
         _Version        =   393216
         Enabled         =   0   'False
         Min             =   1
         Max             =   6
         SelStart        =   1
         Value           =   1
      End
      Begin VB.Label SoundBufLabel 
         Caption         =   " bytes"
         Enabled         =   0   'False
         Height          =   255
         Left            =   4440
         TabIndex        =   64
         Top             =   1320
         Width           =   1095
      End
      Begin VB.Label SizeLabel 
         Caption         =   "Screen Size:"
         Height          =   375
         Left            =   480
         TabIndex        =   0
         Top             =   240
         Width           =   1215
      End
      Begin VB.Label LatencyLabel 
         Caption         =   "0 ms"
         Enabled         =   0   'False
         Height          =   255
         Left            =   4440
         TabIndex        =   38
         Top             =   1680
         Width           =   1095
      End
      Begin VB.Label Label9 
         Caption         =   "Custom Settings"
         Height          =   375
         Left            =   240
         TabIndex        =   1
         Top             =   3240
         Width           =   1335
      End
   End
   Begin MSComctlLib.TabStrip tbsOptions 
      Height          =   4605
      Left            =   120
      TabIndex        =   13
      Top             =   120
      Width           =   5895
      _ExtentX        =   10398
      _ExtentY        =   8123
      MultiRow        =   -1  'True
      _Version        =   393216
      BeginProperty Tabs {1EFB6598-857C-11D1-B16A-00C0F0283628} 
         NumTabs         =   4
         BeginProperty Tab1 {1EFB659A-857C-11D1-B16A-00C0F0283628} 
            Caption         =   "General"
            Key             =   "Group1"
            Object.ToolTipText     =   "Set Emulator Options"
            ImageVarType    =   2
         EndProperty
         BeginProperty Tab2 {1EFB659A-857C-11D1-B16A-00C0F0283628} 
            Caption         =   "Game"
            Object.ToolTipText     =   "Configure Game settings / DIPs"
            ImageVarType    =   2
         EndProperty
         BeginProperty Tab3 {1EFB659A-857C-11D1-B16A-00C0F0283628} 
            Caption         =   "Laserdisc"
            Key             =   "Group2"
            Object.ToolTipText     =   "Set Laserdisc Options"
            ImageVarType    =   2
         EndProperty
         BeginProperty Tab4 {1EFB659A-857C-11D1-B16A-00C0F0283628} 
            Caption         =   "Advanced"
            Object.ToolTipText     =   "Set Advanced Options"
            ImageVarType    =   2
         EndProperty
      EndProperty
      BeginProperty Font {0BE35203-8F91-11CE-9DE3-00AA004BB851} 
         Name            =   "MS Sans Serif"
         Size            =   8.25
         Charset         =   0
         Weight          =   400
         Underline       =   0   'False
         Italic          =   0   'False
         Strikethrough   =   0   'False
      EndProperty
   End
End
Attribute VB_Name = "frmOptions"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False
'Option Explicit
Dim LastTab As Integer 'remembers which tab was last used, so the user doesn't need to reselect the tab each time they open the dialog
Dim AdvancedToggle As Boolean

Private Sub cmdApply_Click()
    UpdateConfig
End Sub

Private Sub cmdCancel_Click()
    Unload Me
End Sub

Private Sub cmdOK_Click()
    UpdateConfig
    Unload Me
End Sub

Private Sub Command1_Click()
    frmOptions.Enabled = False
    ButtonMapForm.Show
End Sub

Private Sub DIPUseDefaultsCheckBox_Click()
Dim usedefaults As Boolean

   If frmOptions.DIPUseDefaultsCheckBox.Value = 1 Then
    usedefaults = True
   Else
    usedefaults = False
   End If
   
    For i = 1 To MaxDIPListBoxes
        frmOptions.DIPlistbox(i - 1).Enabled = Not usedefaults
        frmOptions.DIPlistboxlabel(i - 1).Enabled = Not usedefaults
    Next i
    For i = 1 To MaxDIPCheckBoxes
        frmOptions.DIPcheckbox(i - 1).Enabled = Not usedefaults
    Next i

End Sub

Private Sub EditButton_Click()
 Main.EditFile (MPEGFileTextBox.Text)
End Sub

Private Sub EditKeysButton_Click()
    frmOptions.Enabled = False
    KeyMapForm.Show
'    ConfigureKeys
End Sub

Private Sub FileBrowseButton_Click()
On Error Resume Next
    FileDialog.Flags = cdlOFNHideReadOnly Or cdlOFNExplorer
    FileDialog.CancelError = True
      
   ' Get the MPEG Info filename from the user.
   FileDialog.DialogTitle = "Select MPEG Frame Info File"
   FileDialog.Filter = "MPEG Frame Info Files (*.TXT)|*.txt|All Files (*.*)|*.*"
   
      FileDialog.FileName = ""
      FileDialog.ShowOpen
      If Err = cdlCancel Then Exit Sub
      
      MPEGFileTextBox.Text = FileDialog.FileName
      
End Sub


Private Sub Form_KeyDown(KeyCode As Integer, Shift As Integer)
    Dim i As Integer
    'handle ctrl+tab to move to the next tab
    If Shift = vbCtrlMask And KeyCode = vbKeyTab Then
        i = tbsOptions.SelectedItem.Index
        If i = tbsOptions.Tabs.Count Then
            'last tab so we need to wrap to tab 1
            Set tbsOptions.SelectedItem = tbsOptions.Tabs(1)
        Else
            'increment the tab
            Set tbsOptions.SelectedItem = tbsOptions.Tabs(i + 1)
        End If
    End If
End Sub

Private Sub Form_Load()
    'offset slightly from main form
    Left = MainForm.Left + 600
    Top = MainForm.Top + 600
    
    If LastTab = 0 Then LastTab = 1 'initialize the lasttab holder if necessary
  Set tbsOptions.SelectedItem = tbsOptions.Tabs(LastTab)
  
    
    'refresh the tabs
    tbsOptions_Click
    SeekDelayCheck_Click
    
End Sub

Private Sub Form_Unload(Cancel As Integer)
    
    LastTab = tbsOptions.SelectedItem.Index
    
    MainForm.Enabled = True

End Sub


Private Sub LatencyCheck_Click()
   LatencySlider.Enabled = LatencyCheck.Value
   
   LatencyLabel.Enabled = LatencyCheck.Value
End Sub

Private Sub LatencySlider_Change()
    LatencyLabel.Caption = LatencySlider.Value & " ms"

End Sub


Private Sub LDPlayerList_Change()
    UpdatePlayerOptions
    
End Sub

Sub UpdatePlayerOptions()

'find out what kind of player it is, and show/hide the appropriate options
Dim playertype As String

playertype = UCase(GetFromINI(frmOptions.LDPlayerList.Text, "Type", GetGameListFile()))

Select Case playertype

Case "VIRTUAL"
    SeekTest.Visible = True
    VLDPoptionsFrame.Visible = True
    SerialLDoptionsFrame.Visible = False

Case "SERIAL"
    SeekTest.Visible = False
    VLDPoptionsFrame.Visible = False
    SerialLDoptionsFrame.Visible = True
    
Case Else
    SeekTest.Visible = False
    VLDPoptionsFrame.Visible = False
    SerialLDoptionsFrame.Visible = False
End Select

End Sub


Private Sub LDPlayerList_Click()
    UpdatePlayerOptions
End Sub

Private Sub LDPlayerList_KeyPress(KeyAscii As Integer)
    UpdatePlayerOptions
End Sub


Private Sub SoundBufCheck_Click()
   SoundBufSlider.Enabled = SoundBufCheck.Value
   
   SoundBufLabel.Enabled = SoundBufCheck.Value

End Sub

Private Sub SoundBufSlider_Change()
  SoundBufSlider.Tag = (2 ^ (SoundBufSlider.Value + 7))
  SoundBufSlider.ToolTipText = ""
  SoundBufLabel.Caption = SoundBufSlider.Tag & " bytes"
End Sub

Private Sub tbsOptions_Click()
    
    Dim i As Integer
    'show and enable the selected tab's controls
    'and hide and disable all others
    For i = 0 To tbsOptions.Tabs.Count - 1
        If i = tbsOptions.SelectedItem.Index - 1 Then
            picOptions(i).Left = 210
            picOptions(i).Enabled = True
        Else
            picOptions(i).Left = -20000
            picOptions(i).Enabled = False
        End If
    Next
    
'arrange the options on the General tab if needed
If tbsOptions.SelectedItem = "General" Then

End If
    
'arrange the options on the Game tab if needed
If tbsOptions.SelectedItem = "Game" Then

End If
    
'arrange the options on the LD tab if needed
If tbsOptions.SelectedItem = "Laserdisc" Then

End If
    
End Sub

Private Sub SeekDelayCheck_Click()

If frmOptions.SeekDelayCheck.Value = 1 Then
    frmOptions.SeekDelay.Visible = True
    frmOptions.SeekDelayLabel.Visible = True
    frmOptions.BlankSearchesCheck.Value = 1
    frmOptions.BlankSearchesCheck.Enabled = False
Else
    frmOptions.SeekDelay.Visible = False
    frmOptions.SeekDelayLabel.Visible = False
    frmOptions.BlankSearchesCheck.Enabled = True
End If

End Sub


