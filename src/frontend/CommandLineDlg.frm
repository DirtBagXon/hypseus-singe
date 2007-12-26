VERSION 5.00
Begin VB.Form CommandLineDlg 
   BorderStyle     =   3  'Fixed Dialog
   Caption         =   "Launching Daphne"
   ClientHeight    =   2145
   ClientLeft      =   2760
   ClientTop       =   3750
   ClientWidth     =   7080
   LinkTopic       =   "Form1"
   MaxButton       =   0   'False
   MinButton       =   0   'False
   ScaleHeight     =   2145
   ScaleWidth      =   7080
   ShowInTaskbar   =   0   'False
   StartUpPosition =   1  'CenterOwner
   Begin VB.TextBox Text1 
      Alignment       =   2  'Center
      BackColor       =   &H8000000B&
      Height          =   855
      Left            =   360
      MultiLine       =   -1  'True
      TabIndex        =   2
      Text            =   "CommandLineDlg.frx":0000
      Top             =   360
      Width           =   6495
   End
   Begin VB.CommandButton CancelButton 
      Cancel          =   -1  'True
      Caption         =   "Cancel"
      Height          =   375
      Left            =   3840
      TabIndex        =   1
      Top             =   1560
      Width           =   1215
   End
   Begin VB.CommandButton OKButton 
      Caption         =   "OK"
      Default         =   -1  'True
      Height          =   375
      Left            =   2040
      TabIndex        =   0
      Top             =   1560
      Width           =   1215
   End
End
Attribute VB_Name = "CommandLineDlg"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False

Option Explicit

Private Sub CancelButton_Click()
    CommandLineDlg.Tag = vbCancel
    Me.Hide
End Sub

Private Sub OKButton_Click()
    CommandLineDlg.Tag = vbOK
    Me.Hide
End Sub
