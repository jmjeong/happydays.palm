/*
HappyDays - A Birthdate displayer for the PalmPilot
Copyright (C) 1999-2000 JaeMok Jeong

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
#define MainFormLookup              1001
#define MainFormField               1002
#define MainFormToday               1003
#define MainFormTable               1004
#define MainFormPopupTrigger        1005
#define MainFormAddrCategories      1006
#define MainFormPageUp              1010
#define MainFormPageDown            1011

#define SelectDateString            1099

#define MainFormMenu		        1110
#define	MainFormMenuNotify	        1111
#define	MainFormMenuCleanup	        1112
#define MainFormMenuExport          1113
#define MainFormMenuSl2Ln           1114
#define MainFormMenuLn2Sl           1115
#define	MainFormMenuPref	        1120
#define	MainFormMenuAbout	        1121

#define ViewFormMenu                1151

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
#define PrefFormInternal            1302
#define PrefFormOk					1303
#define PrefFormCancel				1304
#define PrefFormSortDate            1305
#define PrefFormSortName            1306
#define PrefFormNotifyWith          1307

#define NotifySettingForm           1400
#define NotifySettingFormRecordAll  1401
#define NotifySettingFormRecordSlct 1402
#define NotifySettingFormEntryKeep  1403
#define NotifySettingFormEntryModify  1404
#define NotifySettingFormBefore     1406
#define NotifySettingFormPrivate    1407
#define NotifySettingFormAlarm      1408
#define NotifySettingFormTime       1409
#define NotifySettingFormOk         1420
#define NotifySettingFormCancel     1421
#define NotifySettingFormDuration   1422
#define NotifySettingFormBK3    	1423
#define NotifySettingHelpString     1499

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

#define BirthdateForm               1600
#define BirthdateDone               1601
#define BirthdateNotify             1602
#define BirthdateGoto               1603
#define BirthdateFirst              1604
#define BirthdateSecond             1605
#define BirthdateCustom             1606
#define BirthdateDate               1607
#define BirthdateCategory           1608
#define BirthdateOrigin             1609

#define ErrorAlert                  1900
#define CleanupAlert                1901
#define InvalidFormat               1902
#define CustomFieldAlert            1903
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
#define CantFindAddrString          1923
#define CantWritePrefsString        1924
#define SelectTimeString            1925
#define NoTimeString                1926
#define ExportDoneString            1927
#define PrefFormHelpString          1928

#define ExportAlert                 1930
