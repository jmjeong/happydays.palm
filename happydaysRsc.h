/*
HappyDays - A Birthdate displayer for the PalmPilot
Copyright (C) 1999-2001 JaeMok Jeong

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#define StartForm                   999

#define MainForm 			        1000
#define MainFormStart	            1001
#define MainFormTable               1002
#define MainFormPopupTrigger        1003
#define MainFormAddrCategories      1004
#define MainFormScrollBar           1005
#define MainFormName				1006
#define MainFormDate				1007
#define MainFormAge					1008
#define MainFormLookUp				1009

#define SelectDateString            1099

#define MainMenu		            1110
#define	MainMenuNotifyDatebook	    1111
#define	MainMenuCleanupDatebook	    1112
#define MainMenuExport              1113
#define MainMenuSl2Ln               1114
#define MainMenuLn2Sl               1115
#define MainMenuNotifyTodo          1116
#define	MainMenuCleanupTodo	        1117
#define	MainMenuPref	            1120
#define	MainMenuAbout	            1121
#define	MainMenuFont	            1122
#define	MainMenuRescanAddr          1123

#define	MainMenuDispPref	        1130

#define ViewMenu                    1151

#define TextMenu                    1180
#define TextMenuUndo                1181
#define TextMenuCut                 1182
#define TextMenuCopy                1183
#define TextMenuPaste               1184
#define TextMenuSAll                1185
#define TextMenuKBoard              1186
#define TextMenuGHelp               1187

#define AboutForm			        1200
#define AboutHelpString             1201
#define AboutFormOk                 1202
#define AboutFormHelp               1203

#define PrefForm                    1300
#define PrefFormCustomField         1301
#define PrefFormOk					1303
#define PrefFormCancel				1304
#define PrefFormOverrideSystemDate  1306
#define PrefFormDateFmts            1307
#define PrefFormDateTrigger         1308
#define PrefFormNotifyFmts          1309
#define PrefFormNotifyTrigger       1310
#define PrefFormScanNote			1311
#define PrefFormAddress             1312
#define PrefFormAddrTrigger         1313
#define PrefFormRescanTrigger       1314
#define PrefFormAutoRescan          1315
#define PrefFormIgnorePrefix        1316

#define DispPrefForm                1350
#define DispPrefOk                  1351
#define DispPrefCancel              1352
#define DispPrefEmphasize           1353
#define DispPrefExtraInfo           1354

#define DateBookNotifyForm          1400
#define NotifyFormRecordAll  		1401
#define NotifyFormRecordSlct 		1402
#define NotifyFormEntryKeep  		1403
#define NotifyFormEntryModify  		1404
#define NotifyFormPrivate    	 	1406
#define DateBookNotifyFormBefore    1407
#define DateBookNotifyFormAlarm     1408
#define DateBookNotifyFormTime      1409
#define DateBookNotifyFormOk        1420
#define DateBookNotifyFormCancel    1421
#define DateBookNotifyFormDuration  1422
#define DateBookNotifyFormMore    	1423
#define DateBookNotifyFormLabelDays	1424

#define NotifyStringForm		    1450
#define NotifyStringFormOk	        1451
#define DBNoteCheckBox				1452
#define DBNoteField					1453
#define DBNoteScrollBar				1454

#define NotifyStringHelpString      1499

#define Sl2LnForm                   1500
#define Sl2LnFormInput              1501
#define Sl2LnFormResult             1504
#define Sl2LnFormOk                 1505
#define Sl2LnFormConvert            1506

#define Ln2SlForm                   1550
#define Ln2SlFormInput              1551
#define Ln2SlFormInputLeap          1554
#define Ln2SlFormResult             1555
#define Ln2SlFormOk                 1556
#define Ln2SlFormConvert            1557

#define ViewForm               		1600
#define ViewFormDone                1601
#define ViewFormNotifyDB            1602
#define ViewFormNotifyTD            1603
#define ViewFormGoto                1604
#define ViewFormTable               1605
#define ViewFormCategory            1609
#define ViewFormName                1610

#define BirthLeftString				1698
#define BirthPassedString			1699

#define ToDoNotifyForm              1700
#define ToDoNotifyPri1              1701
#define ToDoNotifyPri2              1702
#define ToDoNotifyPri3              1703
#define ToDoNotifyPri4              1704
#define ToDoNotifyPri5              1705
#define ToDoNotifyCategory          1706
#define ToDoPopupTrigger            1707
#define ToDoNotifyFormOk            1708
#define ToDoNotifyFormCancel        1709
#define ToDoNotifyFormMore    	    1710

#define ProgressForm                1800
#define ProgressFormField           1801

#define ErrorAlert                  1900
#define CleanupAlert                1901
#define InvalidFormat               1902

#define CustomErrorHelpString       1904
#define NotifyWarning               1905
#define CleanupAlertHelpString      1906
#define CleanupDone                 1907
#define RemoveConfirmString         1908
#define InvalidDateString           1909
#define SunString                   1910
#define MonString	 	            1911
#define TueString 	                1912
#define WedString 	                1913
#define ThuString 	                1914
#define FriString 	                1915
#define SatString 	                1916

#define Sl2LnFormHelpString         1917
#define NotEnoughMemoryString       1918
#define NotifyCreateString          1919
#define ExistingEntryString         1920
#define UntouchedString             1921
#define ModifiedString              1922
#define SelectTimeString            1924
#define NoTimeString                1925
#define ExportDoneString            1926

#define ExportHeaderString			1929
#define ViewNotExistString			1930
#define DateBookFirstAlertString	1931
#define DateBookNotifyDaysString	1932
#define DateBookString				1933
#define ToDoString					1934
#define FindHeaderString			1935	
#define TypeStr						1936
#define NextDateStr					1937
#define SourceDateStr				1938
#define AgeStr						1939
#define RemainedDayStr				1940
#define AgeFormatString				1941
#define DBNoteString                1942
#define TDNoteString                1943
#define BioRhythmStr				1944
#define BioString					1945
#define ToDoFirstAlertString	    1946

#define JanString					1950		
#define FebString					1951	
#define MarString					1952
#define AprString					1953
#define MayString					1954
#define JunString					1955
#define JulString					1956	
#define AugString					1957
#define SepString					1958
#define OctString					1959
#define NovString					1960
#define DecString					1961

#define WeekInitialString 			1965

#define ExportAlert                 1990
#define AddrRescanAlert             1991

#define ID_BitmapIcon				2000

#define ABOUT_LINE_SPACING			12
#define ABOUT_LINE_SPACING2			18

#define MAXSLOT						24
#define VISIBLESLOT					11
#define FULLWIDTH					151
