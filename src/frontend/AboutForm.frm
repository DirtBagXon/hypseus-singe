VERSION 5.00
Begin VB.Form AboutForm 
   Caption         =   "About Daphne Loader"
   ClientHeight    =   3165
   ClientLeft      =   6315
   ClientTop       =   6930
   ClientWidth     =   5910
   Icon            =   "AboutForm.frx":0000
   LinkTopic       =   "Form1"
   ScaleHeight     =   3165
   ScaleWidth      =   5910
   StartUpPosition =   1  'CenterOwner
   Begin VB.Timer AboutBoxTimer 
      Interval        =   500
      Left            =   240
      Top             =   2640
   End
   Begin VB.CommandButton OKButton 
      Cancel          =   -1  'True
      Caption         =   "&OK"
      Height          =   615
      Left            =   2280
      TabIndex        =   2
      Top             =   2400
      Width           =   1215
   End
   Begin VB.PictureBox AboutPicBox 
      Height          =   1860
      Left            =   120
      ScaleHeight     =   120
      ScaleMode       =   3  'Pixel
      ScaleWidth      =   160
      TabIndex        =   0
      TabStop         =   0   'False
      Top             =   240
      Width           =   2460
   End
   Begin VB.Label AboutLabel 
      Caption         =   "Daphne Loader"
      Height          =   1935
      Left            =   2880
      TabIndex        =   1
      Top             =   240
      Width           =   2535
      WordWrap        =   -1  'True
   End
End
Attribute VB_Name = "AboutForm"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False

Dim piccounter As Integer

Private Sub AboutBoxTimer_Timer()
If piccounter = 1 Then
    picname = App.Path & "\images\lt1.jpg"
    piccounter = 2
Else
    picname = App.Path & "\images\lt2.jpg"
    piccounter = 1
End If
  
If Len(Dir(picname)) > 0 Then AboutPicBox.Picture = LoadPicture(picname)
End Sub

Private Sub Form_Load()
    picname = App.Path & "\images\lt1.jpg"
    If Len(Dir(picname)) > 0 Then AboutPicBox.Picture = LoadPicture(picname)
    AboutLabel.Caption = "Daphne Loader for Windows" & vbCr & "by Warren Ondras" & vbCr & vbCr & _
      "Build 130 - June 2, 2005" & vbCr & vbCr & _
      "Screenshots courtesy of the" & vbCr & "Dragon's Lair Project" & vbCr & vbCr & "www.dragons-lair-project.com"
End Sub

Private Sub Form_Unload(Cancel As Integer)
    MainForm.Enabled = True
End Sub

Private Sub OKButton_Click()
    Unload AboutForm
End Sub
