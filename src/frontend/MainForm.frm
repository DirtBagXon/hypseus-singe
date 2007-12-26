VERSION 5.00
Begin VB.Form MainForm 
   Caption         =   "Daphne Loader 1.3"
   ClientHeight    =   3960
   ClientLeft      =   60
   ClientTop       =   645
   ClientWidth     =   6150
   Icon            =   "MainForm.frx":0000
   LinkTopic       =   "Form1"
   ScaleHeight     =   3960
   ScaleWidth      =   6150
   Begin VB.ListBox GameList 
      Height          =   2595
      Left            =   120
      TabIndex        =   0
      Top             =   120
      Width           =   2895
   End
   Begin VB.CommandButton ExitButton 
      Cancel          =   -1  'True
      Caption         =   "E&xit"
      Height          =   375
      Left            =   120
      TabIndex        =   1
      ToolTipText     =   "Exit"
      Top             =   120
      Width           =   855
   End
   Begin VB.Frame Frame1 
      BorderStyle     =   0  'None
      Caption         =   "Frame1"
      Height          =   3735
      Left            =   3120
      TabIndex        =   2
      Top             =   120
      Width           =   2895
      Begin VB.CommandButton ConfigButton 
         Caption         =   "&Configure"
         Height          =   375
         Left            =   1440
         TabIndex        =   6
         ToolTipText     =   "Configure options for this game"
         Top             =   1920
         Width           =   1215
      End
      Begin VB.CommandButton StartButton 
         Caption         =   "&Start!"
         Height          =   375
         Left            =   120
         TabIndex        =   5
         ToolTipText     =   "Launch Daphne with selected settings"
         Top             =   1920
         Width           =   1215
      End
      Begin VB.PictureBox GamePicBox 
         Height          =   1785
         Left            =   120
         ScaleHeight     =   115
         ScaleMode       =   3  'Pixel
         ScaleWidth      =   160
         TabIndex        =   3
         TabStop         =   0   'False
         Top             =   0
         Width           =   2460
      End
      Begin VB.Image DaphneLogo 
         BorderStyle     =   1  'Fixed Single
         Height          =   915
         Left            =   0
         Picture         =   "MainForm.frx":08CA
         Stretch         =   -1  'True
         Top             =   2400
         Width           =   2715
      End
      Begin VB.Label DaphURLlabel 
         Caption         =   "http://www.daphne-emu.com"
         BeginProperty Font 
            Name            =   "MS Sans Serif"
            Size            =   8.25
            Charset         =   0
            Weight          =   700
            Underline       =   -1  'True
            Italic          =   0   'False
            Strikethrough   =   0   'False
         EndProperty
         ForeColor       =   &H00404040&
         Height          =   255
         Left            =   120
         MouseIcon       =   "MainForm.frx":561C
         MousePointer    =   99  'Custom
         TabIndex        =   4
         ToolTipText     =   "Visit the Daphne home page"
         Top             =   3360
         Width           =   2535
      End
   End
   Begin VB.Line Line1 
      BorderColor     =   &H8000000C&
      X1              =   0
      X2              =   6120
      Y1              =   0
      Y2              =   0
   End
   Begin VB.Menu FileMenu 
      Caption         =   "&File"
      Begin VB.Menu ViewLogMenuItem 
         Caption         =   "&View Daphne Error Log"
         Shortcut        =   ^L
      End
      Begin VB.Menu FileMenuSeparatorItem 
         Caption         =   "-"
      End
      Begin VB.Menu ExitMenuItem 
         Caption         =   "E&xit"
         Shortcut        =   ^X
      End
   End
   Begin VB.Menu GameMenu 
      Caption         =   "&Game"
      Begin VB.Menu StartGameMenuItem 
         Caption         =   "&Start"
         Shortcut        =   ^S
      End
      Begin VB.Menu GameMenuSeparatorItem 
         Caption         =   "-"
      End
      Begin VB.Menu ConfigureGameMenuItem 
         Caption         =   "&Configure"
         Shortcut        =   ^C
      End
   End
   Begin VB.Menu HelpMenu 
      Caption         =   "&Help"
      Begin VB.Menu DaphneWebMenuItem 
         Caption         =   "Daphne &Web Site"
      End
      Begin VB.Menu DaphneDocsMenuItem 
         Caption         =   "Daphne &Documentation"
      End
      Begin VB.Menu HelpSeparatorMenuItem 
         Caption         =   "-"
      End
      Begin VB.Menu AboutMenuItem 
         Caption         =   "&About Daphne Loader"
      End
   End
End
Attribute VB_Name = "MainForm"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False


Private Sub AboutMenuItem_Click()
    MainForm.Enabled = False
    AboutForm.Show
End Sub

Private Sub ConfigButton_Click()
    ConfigureGame
End Sub

Private Sub ConfigureGameMenuItem_Click()
    ConfigureGame
End Sub

Private Sub StartGameMenuItem_Click()
    StartGame
End Sub

Private Sub DaphneDocsMenuItem_Click()
    Main.LaunchDocs
End Sub

Private Sub DaphneWebMenuItem_Click()
    Main.LaunchDaphneURL
End Sub

Private Sub DaphURLlabel_Click()
    Main.LaunchDaphneURL
End Sub

Private Sub DaphneLogo_Click()
    Main.LaunchDaphneURL
End Sub

Private Sub ExitButton_Click()
  Main.ExitApp
End Sub

Private Sub ExitMenuItem_Click()
   Main.ExitApp
End Sub

Private Sub Form_Unload(Cancel As Integer)
   Main.ExitApp
End Sub

Private Sub Form_Load()
  'center the form
    Me.Move (Screen.Width - Me.Width) / 2, (Screen.Height - Me.Height) / 2
    Main.init


Show 'activate the form so we can set the focus to the list box
GameList.SetFocus

End Sub

Private Sub Form_Resize()

'resize the game list
If MainForm.Height > 2000 Then GameList.Height = MainForm.Height - 900
If MainForm.Width > 4000 Then GameList.Width = MainForm.Width - 3000

'move the screenshot / buttons / link
If MainForm.Width > GameList.Width + Frame1.Width + 300 Then
    Frame1.Left = MainForm.Width - Frame1.Width - 150
Else
    'window is too narrow, so let it be cropped on the right
    Frame1.Left = GameList.Width + 150
End If
    
'move the buttons and hide the screenshot if the window gets too short
If MainForm.Height < 4400 Then
    DaphURLlabel.Visible = False
Else
    DaphURLlabel.Visible = True
End If
     
If MainForm.Height < 4150 Then
    DaphneLogo.Visible = False
Else
    DaphneLogo.Visible = True
End If
            
If MainForm.Height < 3100 Then
    GamePicBox.Visible = False
    StartButton.Top = 120
    ConfigButton.Top = 120
Else
    GamePicBox.Visible = True
    StartButton.Top = 1920
    ConfigButton.Top = 1920
End If
End Sub

Private Sub GameList_Click()

SetCurrentGame (GameList.List(GameList.ListIndex))

End Sub

Private Sub GameList_DblClick()
    StartGame
End Sub

Private Sub GameList_KeyPress(KeyAscii As Integer)
 
 If KeyAscii = Asc(vbCr) Then StartGame

End Sub

Private Sub StartButton_Click()
    StartGame
End Sub

Private Sub ViewLogMenuItem_Click()
    Main.ViewErrorLog
End Sub
