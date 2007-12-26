Attribute VB_Name = "INIStuff"

'32 Bit API Declarations - stupid INI and environment variable stuff that should still be built in!
Declare Function GetPrivateProfileString Lib "kernel32" Alias "GetPrivateProfileStringA" (ByVal lpApplicationName As String, lpKeyName As Any, ByVal lpDefault As String, ByVal lpRetunedString As String, ByVal nSize As Long, ByVal lpFileName As String) As Long
Declare Function WritePrivateProfileString Lib "kernel32" Alias "WritePrivateProfileStringA" (ByVal lpApplicationName As String, ByVal lpKeyName As Any, ByVal lpString As Any, ByVal lplFileName As String) As Long

Function GetFromINI(AppName$, KeyName$, FileName$) As String
   Dim RetStr As String
   RetStr = String(255, Chr(0))
   GetFromINI = Left(RetStr, GetPrivateProfileString(AppName$, ByVal KeyName$, "", RetStr, Len(RetStr), FileName$))
End Function

Function GetBoolFromINI(AppName$, KeyName$, FileName$) As Boolean
   Dim RetStr As String
   Dim tmpval As String
   
   RetStr = String(255, Chr(0))
   rc = GetPrivateProfileString(AppName$, ByVal KeyName$, "", RetStr, Len(RetStr), FileName$)
   tmpval = UCase(Left$(RetStr, 1))
   GetBoolFromINI = (tmpval = "Y" Or tmpval = "T")   'Yes or True
End Function

Function GetIntFromINI(AppName$, KeyName$, DefaultValue As Integer, FileName$) As Integer
   
   Dim RetStr As String
      
   GetIntFromINI = DefaultValue
         
   RetStr = String(255, Chr(0))
   rc = GetPrivateProfileString(AppName$, ByVal KeyName$, "", RetStr, Len(RetStr), FileName$)
   If rc > 0 Then GetIntFromINI = Val(RetStr) Else GetIntFromINI = DefaultValue
   
End Function

'useless wrapper so the function names are similar
Sub SaveToINI(AppName$, KeyName$, Value$, FileName$)
    rc = WritePrivateProfileString(AppName$, KeyName$, Value$, FileName$)
End Sub
    

