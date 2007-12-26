VERSION 5.00
Begin VB.Form KeyMapForm 
   BorderStyle     =   3  'Fixed Dialog
   Caption         =   "Customize Keys"
   ClientHeight    =   4710
   ClientLeft      =   2760
   ClientTop       =   3750
   ClientWidth     =   8925
   Icon            =   "KeyMapForm.frx":0000
   LinkTopic       =   "Form1"
   MaxButton       =   0   'False
   MinButton       =   0   'False
   ScaleHeight     =   4710
   ScaleWidth      =   8925
   StartUpPosition =   1  'CenterOwner
   Begin VB.ComboBox KeyList 
      Height          =   315
      Index           =   18
      Left            =   5640
      Style           =   2  'Dropdown List
      TabIndex        =   57
      Top             =   3120
      Width           =   1455
   End
   Begin VB.ComboBox KeyList 
      Height          =   315
      Index           =   17
      Left            =   5640
      Style           =   2  'Dropdown List
      TabIndex        =   56
      Top             =   3480
      Width           =   1455
   End
   Begin VB.ComboBox KeyList2 
      Height          =   315
      Index           =   18
      Left            =   7200
      Style           =   2  'Dropdown List
      TabIndex        =   55
      Top             =   3120
      Width           =   1455
   End
   Begin VB.ComboBox KeyList2 
      Height          =   315
      Index           =   17
      Left            =   7200
      Style           =   2  'Dropdown List
      TabIndex        =   54
      Top             =   3480
      Width           =   1455
   End
   Begin VB.ComboBox KeyList2 
      Height          =   315
      Index           =   16
      Left            =   7200
      Style           =   2  'Dropdown List
      TabIndex        =   53
      Top             =   2760
      Width           =   1455
   End
   Begin VB.ComboBox KeyList2 
      Height          =   315
      Index           =   15
      Left            =   7200
      Style           =   2  'Dropdown List
      TabIndex        =   52
      Top             =   2400
      Width           =   1455
   End
   Begin VB.ComboBox KeyList2 
      Height          =   315
      Index           =   14
      Left            =   7200
      Style           =   2  'Dropdown List
      TabIndex        =   51
      Top             =   2040
      Width           =   1455
   End
   Begin VB.ComboBox KeyList2 
      Height          =   315
      Index           =   13
      Left            =   7200
      Style           =   2  'Dropdown List
      TabIndex        =   50
      Top             =   1680
      Width           =   1455
   End
   Begin VB.ComboBox KeyList2 
      Height          =   315
      Index           =   12
      Left            =   7200
      Style           =   2  'Dropdown List
      TabIndex        =   49
      Top             =   1320
      Width           =   1455
   End
   Begin VB.ComboBox KeyList2 
      Height          =   315
      Index           =   11
      Left            =   7200
      Style           =   2  'Dropdown List
      TabIndex        =   48
      Top             =   960
      Width           =   1455
   End
   Begin VB.ComboBox KeyList2 
      Height          =   315
      Index           =   10
      Left            =   7200
      Style           =   2  'Dropdown List
      TabIndex        =   47
      Top             =   600
      Width           =   1455
   End
   Begin VB.ComboBox KeyList2 
      Height          =   315
      Index           =   9
      Left            =   7200
      Style           =   2  'Dropdown List
      TabIndex        =   46
      Top             =   240
      Width           =   1455
   End
   Begin VB.ComboBox KeyList2 
      Height          =   315
      Index           =   8
      Left            =   2640
      Style           =   2  'Dropdown List
      TabIndex        =   45
      Top             =   3120
      Width           =   1455
   End
   Begin VB.ComboBox KeyList2 
      Height          =   315
      Index           =   7
      Left            =   2640
      Style           =   2  'Dropdown List
      TabIndex        =   44
      Top             =   2760
      Width           =   1455
   End
   Begin VB.ComboBox KeyList2 
      Height          =   315
      Index           =   6
      Left            =   2640
      Style           =   2  'Dropdown List
      TabIndex        =   43
      Top             =   2400
      Width           =   1455
   End
   Begin VB.ComboBox KeyList2 
      Height          =   315
      Index           =   5
      Left            =   2640
      Style           =   2  'Dropdown List
      TabIndex        =   42
      Top             =   2040
      Width           =   1455
   End
   Begin VB.ComboBox KeyList2 
      Height          =   315
      Index           =   4
      Left            =   2640
      Style           =   2  'Dropdown List
      TabIndex        =   41
      Top             =   1680
      Width           =   1455
   End
   Begin VB.ComboBox KeyList2 
      Height          =   315
      Index           =   3
      Left            =   2640
      Style           =   2  'Dropdown List
      TabIndex        =   40
      Top             =   1320
      Width           =   1455
   End
   Begin VB.ComboBox KeyList2 
      Height          =   315
      Index           =   2
      Left            =   2640
      Style           =   2  'Dropdown List
      TabIndex        =   39
      Top             =   960
      Width           =   1455
   End
   Begin VB.ComboBox KeyList2 
      Height          =   315
      Index           =   1
      Left            =   2640
      Style           =   2  'Dropdown List
      TabIndex        =   38
      Top             =   600
      Width           =   1455
   End
   Begin VB.ComboBox KeyList2 
      Height          =   315
      Index           =   0
      Left            =   2640
      Style           =   2  'Dropdown List
      TabIndex        =   37
      Top             =   240
      Width           =   1455
   End
   Begin VB.ComboBox KeyList 
      Height          =   315
      Index           =   16
      Left            =   5640
      Style           =   2  'Dropdown List
      TabIndex        =   36
      Top             =   2760
      Width           =   1455
   End
   Begin VB.ComboBox KeyList 
      Height          =   315
      Index           =   15
      Left            =   5640
      Style           =   2  'Dropdown List
      TabIndex        =   35
      Top             =   2400
      Width           =   1455
   End
   Begin VB.ComboBox KeyList 
      Height          =   315
      Index           =   14
      Left            =   5640
      Style           =   2  'Dropdown List
      TabIndex        =   34
      Top             =   2040
      Width           =   1455
   End
   Begin VB.ComboBox KeyList 
      Height          =   315
      Index           =   13
      Left            =   5640
      Style           =   2  'Dropdown List
      TabIndex        =   33
      Top             =   1680
      Width           =   1455
   End
   Begin VB.ComboBox KeyList 
      Height          =   315
      Index           =   12
      Left            =   5640
      Style           =   2  'Dropdown List
      TabIndex        =   32
      Top             =   1320
      Width           =   1455
   End
   Begin VB.ComboBox KeyList 
      Height          =   315
      Index           =   11
      Left            =   5640
      Style           =   2  'Dropdown List
      TabIndex        =   31
      Top             =   960
      Width           =   1455
   End
   Begin VB.ComboBox KeyList 
      Height          =   315
      Index           =   10
      Left            =   5640
      Style           =   2  'Dropdown List
      TabIndex        =   30
      Top             =   600
      Width           =   1455
   End
   Begin VB.ComboBox KeyList 
      Height          =   315
      Index           =   9
      Left            =   5640
      Style           =   2  'Dropdown List
      TabIndex        =   29
      Top             =   240
      Width           =   1455
   End
   Begin VB.ComboBox KeyList 
      Height          =   315
      Index           =   8
      Left            =   1080
      Style           =   2  'Dropdown List
      TabIndex        =   28
      Top             =   3120
      Width           =   1455
   End
   Begin VB.ComboBox KeyList 
      Height          =   315
      Index           =   7
      Left            =   1080
      Style           =   2  'Dropdown List
      TabIndex        =   27
      Top             =   2760
      Width           =   1455
   End
   Begin VB.ComboBox KeyList 
      Height          =   315
      Index           =   6
      Left            =   1080
      Style           =   2  'Dropdown List
      TabIndex        =   26
      Top             =   2400
      Width           =   1455
   End
   Begin VB.ComboBox KeyList 
      Height          =   315
      Index           =   5
      Left            =   1080
      Style           =   2  'Dropdown List
      TabIndex        =   25
      Top             =   2040
      Width           =   1455
   End
   Begin VB.ComboBox KeyList 
      Height          =   315
      Index           =   4
      Left            =   1080
      Style           =   2  'Dropdown List
      TabIndex        =   24
      Top             =   1680
      Width           =   1455
   End
   Begin VB.ComboBox KeyList 
      Height          =   315
      Index           =   3
      Left            =   1080
      Style           =   2  'Dropdown List
      TabIndex        =   23
      Top             =   1320
      Width           =   1455
   End
   Begin VB.ComboBox KeyList 
      Height          =   315
      Index           =   2
      Left            =   1080
      Style           =   2  'Dropdown List
      TabIndex        =   22
      Top             =   960
      Width           =   1455
   End
   Begin VB.ComboBox KeyList 
      Height          =   315
      Index           =   1
      Left            =   1080
      Style           =   2  'Dropdown List
      TabIndex        =   21
      Top             =   600
      Width           =   1455
   End
   Begin VB.CommandButton DefaultButton 
      Caption         =   "&Reset"
      Height          =   375
      Left            =   2160
      TabIndex        =   3
      Top             =   4080
      Width           =   1215
   End
   Begin VB.ComboBox KeyList 
      Height          =   315
      Index           =   0
      ItemData        =   "KeyMapForm.frx":0442
      Left            =   1080
      List            =   "KeyMapForm.frx":0444
      Style           =   2  'Dropdown List
      TabIndex        =   2
      Top             =   240
      Width           =   1455
   End
   Begin VB.CommandButton CancelButton 
      Cancel          =   -1  'True
      Caption         =   "Cancel"
      Height          =   375
      Left            =   5280
      TabIndex        =   1
      Top             =   4080
      Width           =   1215
   End
   Begin VB.CommandButton OKButton 
      Caption         =   "OK"
      Height          =   375
      Left            =   3720
      TabIndex        =   0
      Top             =   4080
      Width           =   1215
   End
   Begin VB.Label KeyLabel 
      Alignment       =   1  'Right Justify
      AutoSize        =   -1  'True
      Caption         =   "Label1"
      Height          =   195
      Index           =   18
      Left            =   4935
      TabIndex        =   59
      Top             =   3120
      Width           =   480
   End
   Begin VB.Label KeyLabel 
      Alignment       =   1  'Right Justify
      AutoSize        =   -1  'True
      Caption         =   "Label1"
      Height          =   195
      Index           =   17
      Left            =   4935
      TabIndex        =   58
      Top             =   3480
      Width           =   480
   End
   Begin VB.Label KeyLabel 
      Alignment       =   1  'Right Justify
      AutoSize        =   -1  'True
      Caption         =   "Label1"
      Height          =   195
      Index           =   16
      Left            =   4935
      TabIndex        =   20
      Top             =   2760
      Width           =   480
   End
   Begin VB.Label KeyLabel 
      Alignment       =   1  'Right Justify
      AutoSize        =   -1  'True
      Caption         =   "Label1"
      Height          =   195
      Index           =   15
      Left            =   4935
      TabIndex        =   19
      Top             =   2400
      Width           =   480
   End
   Begin VB.Label KeyLabel 
      Alignment       =   1  'Right Justify
      AutoSize        =   -1  'True
      Caption         =   "Label1"
      Height          =   195
      Index           =   14
      Left            =   4935
      TabIndex        =   18
      Top             =   2040
      Width           =   480
   End
   Begin VB.Label KeyLabel 
      Alignment       =   1  'Right Justify
      AutoSize        =   -1  'True
      Caption         =   "Label1"
      Height          =   195
      Index           =   13
      Left            =   4935
      TabIndex        =   17
      Top             =   1680
      Width           =   480
   End
   Begin VB.Label KeyLabel 
      Alignment       =   1  'Right Justify
      AutoSize        =   -1  'True
      Caption         =   "Label1"
      Height          =   195
      Index           =   12
      Left            =   4935
      TabIndex        =   16
      Top             =   1320
      Width           =   480
   End
   Begin VB.Label KeyLabel 
      Alignment       =   1  'Right Justify
      AutoSize        =   -1  'True
      Caption         =   "Label1"
      Height          =   195
      Index           =   11
      Left            =   4935
      TabIndex        =   15
      Top             =   960
      Width           =   480
   End
   Begin VB.Label KeyLabel 
      Alignment       =   1  'Right Justify
      AutoSize        =   -1  'True
      Caption         =   "Label1"
      Height          =   195
      Index           =   10
      Left            =   4935
      TabIndex        =   14
      Top             =   600
      Width           =   480
   End
   Begin VB.Label KeyLabel 
      Alignment       =   1  'Right Justify
      AutoSize        =   -1  'True
      Caption         =   "Label1"
      Height          =   195
      Index           =   9
      Left            =   4935
      TabIndex        =   13
      Top             =   240
      Width           =   480
   End
   Begin VB.Label KeyLabel 
      Alignment       =   1  'Right Justify
      AutoSize        =   -1  'True
      Caption         =   "Label1"
      Height          =   195
      Index           =   8
      Left            =   495
      TabIndex        =   12
      Top             =   3120
      Width           =   480
   End
   Begin VB.Label KeyLabel 
      Alignment       =   1  'Right Justify
      AutoSize        =   -1  'True
      Caption         =   "Label1"
      Height          =   195
      Index           =   7
      Left            =   495
      TabIndex        =   11
      Top             =   2760
      Width           =   480
   End
   Begin VB.Label KeyLabel 
      Alignment       =   1  'Right Justify
      AutoSize        =   -1  'True
      Caption         =   "Label1"
      Height          =   195
      Index           =   6
      Left            =   495
      TabIndex        =   10
      Top             =   2400
      Width           =   480
   End
   Begin VB.Label KeyLabel 
      Alignment       =   1  'Right Justify
      AutoSize        =   -1  'True
      Caption         =   "Label1"
      Height          =   195
      Index           =   5
      Left            =   495
      TabIndex        =   9
      Top             =   2040
      Width           =   480
   End
   Begin VB.Label KeyLabel 
      Alignment       =   1  'Right Justify
      AutoSize        =   -1  'True
      Caption         =   "Label1"
      Height          =   195
      Index           =   4
      Left            =   495
      TabIndex        =   8
      Top             =   1680
      Width           =   480
   End
   Begin VB.Label KeyLabel 
      Alignment       =   1  'Right Justify
      AutoSize        =   -1  'True
      Caption         =   "Label1"
      Height          =   195
      Index           =   3
      Left            =   495
      TabIndex        =   7
      Top             =   1320
      Width           =   480
   End
   Begin VB.Label KeyLabel 
      Alignment       =   1  'Right Justify
      AutoSize        =   -1  'True
      Caption         =   "Label1"
      Height          =   195
      Index           =   2
      Left            =   495
      TabIndex        =   6
      Top             =   960
      Width           =   480
   End
   Begin VB.Label KeyLabel 
      Alignment       =   1  'Right Justify
      AutoSize        =   -1  'True
      Caption         =   "Label1"
      Height          =   195
      Index           =   1
      Left            =   495
      TabIndex        =   5
      Top             =   600
      Width           =   480
   End
   Begin VB.Label KeyLabel 
      Alignment       =   1  'Right Justify
      AutoSize        =   -1  'True
      Caption         =   "Label1"
      Height          =   195
      Index           =   0
      Left            =   495
      TabIndex        =   4
      Top             =   240
      Width           =   480
   End
End
Attribute VB_Name = "KeyMapForm"
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
        KeyList(i - 1).ListIndex = keycodearray(keyArray(0, i - 1))
        KeyList2(i - 1).ListIndex = keycodearray(keyArray(1, i - 1))
    Next i
End Sub

Private Sub Form_Load()
    Dim thisLabel As String
    Dim i As Integer
    Dim j As Integer
    
    For i = 1 To MaxKeys
        For j = 1 To numdefinedkeys
            KeyMapForm.KeyList(i - 1).AddItem (keynamearray(j - 1, 1))
            KeyMapForm.KeyList2(i - 1).AddItem (keynamearray(j - 1, 1))
        Next j
        
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
        keyArray(0, i) = keynamearray(KeyList(i).ListIndex, 2)
        keyArray(1, i) = keynamearray(KeyList2(i).ListIndex, 2)
    Next i
    
    UpdateKeys
    Unload Me
End Sub

