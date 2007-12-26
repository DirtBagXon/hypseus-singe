VERSION 5.00
Begin VB.Form ButtonMapForm 
   BorderStyle     =   3  'Fixed Dialog
   Caption         =   "Customize Joystick Buttons"
   ClientHeight    =   3870
   ClientLeft      =   2760
   ClientTop       =   3750
   ClientWidth     =   5895
   Icon            =   "ButtonMapForm.frx":0000
   LinkTopic       =   "Form1"
   MaxButton       =   0   'False
   MinButton       =   0   'False
   ScaleHeight     =   3870
   ScaleWidth      =   5895
   StartUpPosition =   1  'CenterOwner
   Begin VB.ComboBox KeyList 
      Height          =   315
      Index           =   18
      Left            =   3960
      Style           =   2  'Dropdown List
      TabIndex        =   38
      Top             =   2400
      Width           =   1695
   End
   Begin VB.ComboBox KeyList 
      Height          =   315
      Index           =   17
      Left            =   3960
      Style           =   2  'Dropdown List
      TabIndex        =   37
      Top             =   2760
      Width           =   1695
   End
   Begin VB.ComboBox KeyList 
      Height          =   315
      Index           =   16
      Left            =   3960
      Style           =   2  'Dropdown List
      TabIndex        =   36
      Top             =   2040
      Width           =   1695
   End
   Begin VB.ComboBox KeyList 
      Height          =   315
      Index           =   15
      Left            =   3960
      Style           =   2  'Dropdown List
      TabIndex        =   35
      Top             =   1680
      Width           =   1695
   End
   Begin VB.ComboBox KeyList 
      Height          =   315
      Index           =   14
      Left            =   3960
      Style           =   2  'Dropdown List
      TabIndex        =   34
      Top             =   1320
      Width           =   1695
   End
   Begin VB.ComboBox KeyList 
      Height          =   315
      Index           =   13
      Left            =   3960
      Style           =   2  'Dropdown List
      TabIndex        =   33
      Top             =   960
      Width           =   1695
   End
   Begin VB.ComboBox KeyList 
      Height          =   315
      Index           =   12
      Left            =   3960
      Style           =   2  'Dropdown List
      TabIndex        =   32
      Top             =   600
      Width           =   1695
   End
   Begin VB.ComboBox KeyList 
      Height          =   315
      Index           =   11
      Left            =   3960
      Style           =   2  'Dropdown List
      TabIndex        =   31
      Top             =   240
      Width           =   1695
   End
   Begin VB.ComboBox KeyList 
      Height          =   315
      Index           =   10
      Left            =   960
      Style           =   2  'Dropdown List
      TabIndex        =   30
      Top             =   2400
      Width           =   1695
   End
   Begin VB.ComboBox KeyList 
      Height          =   315
      Index           =   9
      Left            =   960
      Style           =   2  'Dropdown List
      TabIndex        =   29
      Top             =   2040
      Width           =   1695
   End
   Begin VB.ComboBox KeyList 
      Height          =   315
      Index           =   8
      Left            =   960
      Style           =   2  'Dropdown List
      TabIndex        =   28
      Top             =   1680
      Width           =   1695
   End
   Begin VB.ComboBox KeyList 
      Height          =   315
      Index           =   7
      Left            =   960
      Style           =   2  'Dropdown List
      TabIndex        =   27
      Top             =   1320
      Width           =   1695
   End
   Begin VB.ComboBox KeyList 
      Height          =   315
      Index           =   6
      Left            =   960
      Style           =   2  'Dropdown List
      TabIndex        =   26
      Top             =   960
      Width           =   1695
   End
   Begin VB.ComboBox KeyList 
      Height          =   315
      Index           =   5
      Left            =   960
      Style           =   2  'Dropdown List
      TabIndex        =   25
      Top             =   600
      Width           =   1695
   End
   Begin VB.ComboBox KeyList 
      Height          =   315
      Index           =   4
      Left            =   960
      Style           =   2  'Dropdown List
      TabIndex        =   24
      Top             =   240
      Width           =   1695
   End
   Begin VB.CommandButton DefaultButton 
      Caption         =   "&Reset"
      Height          =   375
      Left            =   720
      TabIndex        =   3
      Top             =   3360
      Width           =   1215
   End
   Begin VB.CommandButton CancelButton 
      Cancel          =   -1  'True
      Caption         =   "Cancel"
      Height          =   375
      Left            =   3840
      TabIndex        =   1
      Top             =   3360
      Width           =   1215
   End
   Begin VB.CommandButton OKButton 
      Caption         =   "OK"
      Height          =   375
      Left            =   2280
      TabIndex        =   0
      Top             =   3360
      Width           =   1215
   End
   Begin VB.ComboBox KeyList 
      Enabled         =   0   'False
      Height          =   315
      Index           =   0
      Left            =   1920
      Style           =   2  'Dropdown List
      TabIndex        =   2
      Top             =   3360
      Visible         =   0   'False
      Width           =   390
   End
   Begin VB.ComboBox KeyList 
      Enabled         =   0   'False
      Height          =   315
      Index           =   1
      Left            =   2280
      Style           =   2  'Dropdown List
      TabIndex        =   21
      Top             =   3360
      Visible         =   0   'False
      Width           =   390
   End
   Begin VB.ComboBox KeyList 
      Enabled         =   0   'False
      Height          =   315
      Index           =   2
      Left            =   2640
      Style           =   2  'Dropdown List
      TabIndex        =   22
      Top             =   3360
      Visible         =   0   'False
      Width           =   390
   End
   Begin VB.ComboBox KeyList 
      Enabled         =   0   'False
      Height          =   315
      Index           =   3
      Left            =   3000
      Style           =   2  'Dropdown List
      TabIndex        =   23
      Top             =   3360
      Visible         =   0   'False
      Width           =   390
   End
   Begin VB.Label KeyLabel 
      Alignment       =   1  'Right Justify
      AutoSize        =   -1  'True
      Caption         =   "Label1"
      Height          =   195
      Index           =   18
      Left            =   3375
      TabIndex        =   40
      Top             =   2400
      Width           =   480
   End
   Begin VB.Label KeyLabel 
      Alignment       =   1  'Right Justify
      AutoSize        =   -1  'True
      Caption         =   "Label1"
      Height          =   195
      Index           =   17
      Left            =   3375
      TabIndex        =   39
      Top             =   2760
      Width           =   480
   End
   Begin VB.Label KeyLabel 
      Alignment       =   1  'Right Justify
      AutoSize        =   -1  'True
      Caption         =   "Label1"
      Height          =   195
      Index           =   16
      Left            =   3375
      TabIndex        =   20
      Top             =   2040
      Width           =   480
   End
   Begin VB.Label KeyLabel 
      Alignment       =   1  'Right Justify
      AutoSize        =   -1  'True
      Caption         =   "Label1"
      Height          =   195
      Index           =   15
      Left            =   3375
      TabIndex        =   19
      Top             =   1680
      Width           =   480
   End
   Begin VB.Label KeyLabel 
      Alignment       =   1  'Right Justify
      AutoSize        =   -1  'True
      Caption         =   "Label1"
      Height          =   195
      Index           =   14
      Left            =   3375
      TabIndex        =   18
      Top             =   1320
      Width           =   480
   End
   Begin VB.Label KeyLabel 
      Alignment       =   1  'Right Justify
      AutoSize        =   -1  'True
      Caption         =   "Label1"
      Height          =   195
      Index           =   13
      Left            =   3375
      TabIndex        =   17
      Top             =   960
      Width           =   480
   End
   Begin VB.Label KeyLabel 
      Alignment       =   1  'Right Justify
      AutoSize        =   -1  'True
      Caption         =   "Label1"
      Height          =   195
      Index           =   12
      Left            =   3375
      TabIndex        =   16
      Top             =   600
      Width           =   480
   End
   Begin VB.Label KeyLabel 
      Alignment       =   1  'Right Justify
      AutoSize        =   -1  'True
      Caption         =   "Label1"
      Height          =   195
      Index           =   11
      Left            =   3360
      TabIndex        =   15
      Top             =   240
      Width           =   480
   End
   Begin VB.Label KeyLabel 
      Alignment       =   1  'Right Justify
      AutoSize        =   -1  'True
      Caption         =   "Label1"
      Height          =   195
      Index           =   10
      Left            =   375
      TabIndex        =   14
      Top             =   2400
      Width           =   480
   End
   Begin VB.Label KeyLabel 
      Alignment       =   1  'Right Justify
      AutoSize        =   -1  'True
      Caption         =   "Label1"
      Height          =   195
      Index           =   9
      Left            =   375
      TabIndex        =   13
      Top             =   2040
      Width           =   480
   End
   Begin VB.Label KeyLabel 
      Alignment       =   1  'Right Justify
      AutoSize        =   -1  'True
      Caption         =   "Label1"
      Height          =   195
      Index           =   8
      Left            =   375
      TabIndex        =   12
      Top             =   1680
      Width           =   480
   End
   Begin VB.Label KeyLabel 
      Alignment       =   1  'Right Justify
      AutoSize        =   -1  'True
      Caption         =   "Label1"
      Height          =   195
      Index           =   7
      Left            =   375
      TabIndex        =   11
      Top             =   1320
      Width           =   480
   End
   Begin VB.Label KeyLabel 
      Alignment       =   1  'Right Justify
      AutoSize        =   -1  'True
      Caption         =   "Label1"
      Height          =   195
      Index           =   6
      Left            =   375
      TabIndex        =   10
      Top             =   960
      Width           =   480
   End
   Begin VB.Label KeyLabel 
      Alignment       =   1  'Right Justify
      AutoSize        =   -1  'True
      Caption         =   "Label1"
      Height          =   195
      Index           =   5
      Left            =   375
      TabIndex        =   9
      Top             =   600
      Width           =   480
   End
   Begin VB.Label KeyLabel 
      Alignment       =   1  'Right Justify
      AutoSize        =   -1  'True
      Caption         =   "Label1"
      Height          =   195
      Index           =   4
      Left            =   375
      TabIndex        =   8
      Top             =   240
      Width           =   480
   End
   Begin VB.Label KeyLabel 
      Alignment       =   1  'Right Justify
      AutoSize        =   -1  'True
      Caption         =   "Label1"
      Height          =   195
      Index           =   3
      Left            =   735
      TabIndex        =   7
      Top             =   1200
      Visible         =   0   'False
      Width           =   15
   End
   Begin VB.Label KeyLabel 
      Alignment       =   1  'Right Justify
      AutoSize        =   -1  'True
      Caption         =   "Label1"
      Height          =   195
      Index           =   2
      Left            =   735
      TabIndex        =   6
      Top             =   840
      Visible         =   0   'False
      Width           =   15
   End
   Begin VB.Label KeyLabel 
      Alignment       =   1  'Right Justify
      AutoSize        =   -1  'True
      Caption         =   "Label1"
      Height          =   195
      Index           =   1
      Left            =   735
      TabIndex        =   5
      Top             =   480
      Visible         =   0   'False
      Width           =   15
   End
   Begin VB.Label KeyLabel 
      Alignment       =   1  'Right Justify
      AutoSize        =   -1  'True
      Caption         =   "Label1"
      Height          =   195
      Index           =   0
      Left            =   735
      TabIndex        =   4
      Top             =   120
      Visible         =   0   'False
      Width           =   15
   End
End
Attribute VB_Name = "ButtonMapForm"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False

Option Explicit

Private Sub CancelButton_Click()
    Unload Me
End Sub

Private Sub DefaultButton_Click()
    Dim i As Integer
    
    For i = 1 To MaxKeys
        KeyList(i - 1).ListIndex = keyArray(2, i - 1)
    Next i
End Sub


Private Sub Form_Load()
    Dim thisLabel As String
    Dim i As Integer
    Dim j As Integer
    
'    Me.Icon = KeyMapForm.Icon
    
    For i = 1 To MaxKeys
        KeyList(i - 1).AddItem ("(none)")
        For j = 1 To 10
            KeyList(i - 1).AddItem ("Button " & j)
        Next j
        
        'get the display name
        thisLabel = GetFromINI("InputKeys", "Key" & i & "Name", GetGameListFile)
        KeyLabel(i - 1).Caption = thisLabel
    Next i
    
    DefaultButton_Click
End Sub

Private Sub Form_Unload(Cancel As Integer)
    frmOptions.Enabled = True 're-enable the form that called this one
End Sub

Private Sub OKButton_Click()
    Dim i As Integer
    
    For i = 0 To (MaxKeys - 1)
        keyArray(2, i) = KeyList(i).ListIndex
    Next i
    
    UpdateKeys
    Unload Me
End Sub

