// HappyDays - A birthday display for the PalmPilot
//
//      Copyright(c) 1999-2000 JaeMok Jeong
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

#include "happydaysRsc.h"

TRANSLATION "ENGLISH"
BEGIN
    "Translator" = "English version"
    "Translator's email" = ""

    "long_notifySettingString" = ""\
    "* Records\n"\
    "Tab 'all' if you'd like to "\
    "make notification entries created for all of your events in this"\
    " category."\
    "Tab 'selected' if only the currently selected record should be "\
    "taken into account.\n\n"\
    "* Existing\n"\
    "Tap 'keep' if existing HappyDays entries "\
    "should be left untouched. Tap 'modify' if you'd like "\
    "these entries to be changed.\n\n"\
    "* Alarm\n"\
    "will set the alarm notification in your datebook entry. Set "\
	"the number of days in advance you'd like to get notified.\n\n"\
    "* Time\n"\
	"Time of the event\n\n"\
    "* Repeat for\n"\
    "is the interval of the notification record. "\
    "-1 means repeat forever in case of solar event, and "\
    "means repeat for 5 years in lunar event for convenience.\n"

	"long_notifySettingMoreString" = ""\
	"For AN Icon Number, consult Action Names manual. DateBk3Icon is "\
	"read from MemoPad. If you don't have DateBk3 Icon entry in MemoPad, "\
	"nothing is displayed.\n"

    "long_customErrorString" = ""\
    "'Birthday' is the default custom field name."\
    "To set a different default custom field"\
    " name, tap 'Preferences'.\n\n"\
    "IMPORTANT: You need to launch your Addressbook and select 'Options/"\
    "Rename Custom Fields' to rename a custom field to 'Birthday'"\
    " or whatever you set as a custom field name in the Preferences of "\
    "HappyDays.\n"

    "long_aboutString" = ""\
    "'HappyDays' will help you remember the birthdays and anniversaries "\
    "of your friends and family members, as well as other events."\
    "\n\nUse one of the custom fields in Address Book application "\
    "and rename it to 'Birthday'."\
    "\n\nExamples:\n"\
    " Last Name: Jeong\n"\
    " First name: JaeMok\n"\
    " ...\n"\
    " Birthday: -)12/4/70\n\n"\
    "Formats:\n"\
    " [-)|#)]Date\n"\
    " -) means lunar calendar\n"\
    " #) means lunar leap month\n\n"\
    "Date format is automatically read from the system preferences. "\
    "You can omit the year field. Year input below 100 is assumed to be "\
    "year in the 20th century, If you are not sure of the year format, "\
    "just write it in form of YYYY(e.g. 1970).\n\n"\
    "To set multiple entry, refer the manual.\n"

    "long_cleanupAlertString" = ""\
    "Notification entries are considered HappyDays if they have "\
	"{HappyDays ID} in their memo field. "\ 	
    "If you use this format in your own "\
    "entries, this record might also be removed by CleanUp.\n"

    "long_prefFormString" = ""\
	"* Instead of system date...\n"\
	"Control the interpretation of the AddressDB Birthday custom field. "\
	"By default this is the same as the system date format preference.\n\n"\
	"* Scan note field\n"\
	"To read note field in Address Book, check this option.\n\n"\
    "* Emphasize Lunar Date\n"\
    "If checkbox is on, the dashed line is drawed beneath lunar date.\n\n"\
	"* Notify format\n"\
	"The description format used for notification\n\n"\
    "* Custom field\n"\
    "The name of custom fields in Address Book used for INPUT\n\n"\
    "* HappyDays ID\n"\
    "Used for Notify and Cleanup\n"

    "long_removeConfirmString" = ""\
    "Are you sure you want to remove all "\
    "HappyDays notify entries?"

    "long_sl2lnString" = ""\
        "-) means plain month in lunar\n"\
        "#) means leap month in lunar"

    "long_cantFindAddrString" = ""\
    "Can't find custom address book field: '%s'\n"\
    "Please rename a custom field of your address book."
END

TRANSLATION "KOREAN"
BEGIN
    "Translator" = "ÇÑ±¹¾î ¹öÁ¯"
    "Translator's email" = ""

    "long_notifySettingString" = ""\
    "* ·¹ÄÚµå\n"\
    "¼±ÅÃµÈ ºÐ·ù¿¡ ÀÖ´Â ¸ðµç ÀÌº¥Æ®¸¦ ÀÏÁ¤°ü¸®·Î ¿Å±â°í ½ÍÀº °æ¿ì¿¡´Â "\
    "'ÀüºÎ'¸¦ ´©¸£¼¼¿ä. ¼±ÅÃµÈ ·¹ÄÚµå¸¸À» ¿Å±â°í ½ÍÀº °æ¿ì¿¡´Â "\
    "'¼±ÅÃ'À» ´©¸£¼¼¿ä.\n\n"\
    "* Á¸ÀçÇÏ´Â ·¹ÄÚµå\n"\
    "Á¸ÀçÇÏ´Â HappyDays Ç×¸ñÀ» À¯ÁöÇÏ°í ½ÍÀº °æ¿ì¿¡´Â 'À¯Áö'¸¦, "\
    "¼öÁ¤ÇÏ°í ½ÍÀº °æ¿ì¿¡´Â '¼öÁ¤'À» ´©¸£¼¼¿ä.\n\n"\
    "* °³ÀÎ¿ë\n"\
    "¾Ë¸² ¼³Á¤ÇÒ ÀÏÁ¤°ü¸® Ç×¸ñÀ» '°³ÀÎ¿ë'À¸·Î ¸¸µé°í ½ÍÀ» ¶§ ¼±ÅÃ.\n\n"\
    "* ¾Ë¶÷\n"\
    "ÀÏÁ¤°ü¸® Ç×¸ñ¿¡ ¾Ë¶÷À» ¼³Á¤. ¸î ÀÏÀü¿¡ ¾Ë¶÷ÀÌ ¿ï¸±Áö ¼³Á¤.\n\n"\
	"* ½Ã°£\n"\
	"ÀÏÁ¤°ü¸® Ç×¸ñÀÇ ½Ã°£À» ¼³Á¤.\n\n"\
    "* ¸î ³â°£\n"\
    "¸î ³â¿¡ °ÉÃÄ ¾Ë¸² ·¹ÄÚµå¸¦ ¸¸µé°ÍÀÎ°¡¸¦ ¼³Á¤. "\
    "-1À» ÀÔ·ÂÇÑ °æ¿ì¿¡´Â ¾ç·ÂÀÇ °æ¿ì '¿µ¿øÈ÷'¸¦, À½·ÂÀÎ °æ¿ì¿£ "\
    "ÆíÀÇ»ó 5³â°£ ¹Ýº¹ÇÏ¿© ·¹ÄÚµå¸¦ ¸¸µç´Ù.\n"

	"long_notifySettingMoreString" = ""\
	"AN »ç¿ëÀÚ´Â Action Names ¸Å´º¾óÀ» ÂüÁ¶ÇÏ¿© ¼ýÀÚ¸¦ ÀÔ·ÂÇÏ¸é µÊ. "\
	"DateBk3 ¾ÆÀÌÄÜÀº ¸Þ¸ðÀåÀ¸·ÎºÎÅÍ ÀÐ¾î¼­ È­¸é¿¡ Ç¥½ÃÇÕ´Ï´Ù. ¸Þ¸ðÀå¿¡ "\
	"¾ÆÀÌÄÜ µ¥ÀÌÅÍ°¡ ¾ø´Â °æ¿ì¿¡´Â ¾Æ¹«°Íµµ Ç¥½ÃµÇÁö ¾Ê½À´Ï´Ù.\n"

    "long_customErrorString" = ""\
    "'Birthday'°¡ ÃÊ±â »ç¿ëÀÚÁ¤ÀÇ ÇÊµå ÀÌ¸§ÀÔ´Ï´Ù."\
    "´Ù¸¥ ÀÌ¸§À» ¼³Á¤ÇÏ°í ½ÍÀº °æ¿ì "\
    "'¼³Á¤'À» ´©¸£¼¼¿ä.\n\n"\
    "Á¤º¸: 'Birthday' »ç¿ëÀÚÁ¤ÀÇ ÇÊµå¸¦ ¹Ù²Ù±â À§ÇØ¼­´Â ÁÖ¼Ò·ÏÀ» ¼öÇà½ÃÄÑ "\
    "'Options/Rename Custom Fields'À» ¼±ÅÃÇÑ ´ÙÀ½ ¼öÁ¤ÇÏ¸é µË´Ï´Ù."\
    "È¤Àº HappyDaysÀÇ ¼³Á¤ Ç×¸ñÀÇ »ç¿ëÀÚÁ¤ÀÇ ÇÊµå¸¦ ÀûÀýÇÏ°Ô ¼öÁ¤ÇÏ¼¼¿ä.\n"

    "long_aboutString" = ""\
    "'HappyDays'´Â Ä£±¸³ª °¡Á·ÀÇ »ýÀÏÀÌ³ª ±â³äÀÏÀ» ±â¾ïÇÏ´Â °ÍÀ» µµ¿Í"\
    "ÁÝ´Ï´Ù. ´õ¿íÀÌ Á¦»ç³ª ´Ù¸¥ ÀÌº¥Æ® Ç×¸ñµµ °ü¸®ÇÒ ¼ö ÀÖ½À´Ï´Ù.\n\n"\
    "ÁÖ¼Ò·ÏÀÇ »ç¿ëÀÚÁ¤ÀÇ ÇÊµåÁß ÇÏ³ª¸¦ 'Birthday'·Î ¼öÁ¤ÇÏ¼¼¿ä."\
    "\n\n¿¹:\n"\
    " Last Name: Á¤\n"\
    " First name: Àç¸ñ\n"\
    " ...\n"\
    " Birthday: -)12/4/70\n\n"\
    "Çü½Ä:\n"\
    " [-)|#)]³¯Â¥\n"\
    " -) À½·Â Æò´Þ\n"\
    " #) À½·Â À±´Þ\n\n"\
    "³¯Â¥ Çü½ÄÀº ½Ã½ºÅÛ ¼³Á¤È­ÀÏ¿¡¼­ ÀÚµ¿À¸·Î ÀÐ¾î¿É´Ï´Ù. ±×·¯¹Ç·Î "\
    "ÀÔ·ÂµÇ´Â ³¯Â¥ÀÇ Çü½ÄÀ» ½Ã½ºÅÛ°ú ÀÏÄ¡½ÃÅ°¼¼¿ä. '³âµµ' ºÎºÐÀº »ý·«"\
    "°¡´ÉÇÕ´Ï´Ù. ³âµµ°¡ 100º¸´Ù ÀûÀº °æ¿ì¿¡´Â 19xx³â´ë¸¦ ¶æÇÕ´Ï´Ù."\
    "È®½ÅÀÌ µéÁö ¾Ê´Â °æ¿ì ±×³É '1970'Ã³·³ 4ÀÚ¸®·Î ÀÔ·ÂÇÏ¼¼¿ä.\n\n"\
    "ÀÚ¼¼ÇÑ °Ç ¸Å´º¾óÀ» ÂüÁ¶ÇÏ¼¼¿ä.\n"

    "long_cleanupAlertString" = ""\
    "¾Ë¸² ·¹ÄÚµåÀÇ ¸Þ¸ð ÇÊµå¿¡ {HD ½Äº°ÀÚ}°¡ ÀÖ´Â °æ¿ì "\
    "HappyDays ·¹ÄÚµå·Î "\
    "°£ÁÖÇÏ°í »èÁ¦ÇÕ´Ï´Ù. (ÁÖÀÇ ¿ä¸Á)\n"

    "long_prefFormString" = ""\
	"* ½Ã½ºÅÛ ³¯Â¥ ´ë½Å...\n"\
	"ÁÖ¼Ò·ÏÀÇ Birthday »ç¿ëÀÚ Á¤ÀÇ ÇÊµå¿¡¼­ ÀÐ¾î¿À´Â ³¯Â¥ Çü½ÄÀ» "\
	"¾î¶»°Ô ÇØ¼®ÇÒÁö °áÁ¤ÇÕ´Ï´Ù. ÃÊ±â°ªÀº ½Ã½ºÅÛÀÇ ³¯Â¥ Çü½ÄÀ» "\
	"»ç¿ëÇÕ´Ï´Ù.\n\n"\
	"* ³ëÆ® ÇÊµå ÀÐ±â\n"\
	"ÁÖ¼Ò·ÏÀÇ ³ëÆ® ÇÊµå¸¦ ÀÐ±â À§ÇØ¼­, ÀÌ ¿É¼ÇÀ» ÄÑ½Ê½Ã¿À. ³ëÆ® ÇÊµåÀÇ "\
	"ÀÔ·Â Çü½ÄÀº ¸Å´º¾óÀ» ÂüÁ¶ÇÏ¼¼¿ä.\n\n"\
    "* À½·Â°­Á¶\n"\
    "¼±ÅÃµÈ °æ¿ì À½·Â»ýÀÏ ¹Ø¿¡ Á¡¼±ÀÌ ±×¾îÁý´Ï´Ù.\n\n"\
	"* ¾Ë¸² ·¹ÄÚµå Çü½Ä\n"\
	"¾Ë¸² ·¹ÄÚµå¸¦ ¸¸µé ¶§ »ç¿ëµÇ´Â ±â¼úÇü½Ä.\n\n"\
    "* »ç¿ëÀÚÁ¤ÀÇ ÇÊµå\n"\
    "ÀÔ·ÂÀ¸·Î »ç¿ëµÇ´Â ÁÖ¼Ò·ÏÀÇ »ç¿ëÀÚÁ¤ÀÇ ÇÊµå ÀÌ¸§.\n\n"\
    "* HD ½Äº°ÀÚ\n"\
    "HappyDays ½Äº°ÀÚ. ¾Ë¸²°ú ·¹ÄÚµå Á¤¸®¿¡ »ç¿ë.\n\n"

    "long_removeConfirmString" = ""\
    "¸ðµç HappyDays ¾Ë¸² Ç×¸ñÀ» »èÁ¦ÇÏ±æ ¿øÇÏ½Ê´Ï±î?"

    "long_sl2lnString" = ""\
        "-)´Â À½·Â Æò´ÞÀ» ÀÇ¹ÌÇÕ´Ï´Ù.\n"\
        "#)Àº À½·Â À±´ÞÀ» ÀÇ¹ÌÇÕ´Ï´Ù."

    "long_cantFindAddrString" = ""\
        "ÁÖ¼Ò·Ï¿¡¼­ »ç¿ëÀÚÁ¤ÀÇ ÇÊµå¸¦ ¸ø Ã£À½: '%s'\n"\
        "ÁÖ¼Ò·ÏÀÇ »ç¿ëÀÚÁ¤ÀÇ ÇÊµå Ç×¸ñ ÀÌ¸§À» º¯°æÇÏ¼¼¿ä."

    "Select Time" = "¾Ë¶÷ ½Ã°£"
    "No Time" = "ÁöÁ¤¾ÈµÊ"
    "Invalid input date" = "³¯Â¥ ÀÔ·ÂÀÌ Àß¸øµÇ¾ú½À´Ï´Ù."
    "Export to Memo Pad is completed." =
                "¸Þ¸ðÀåÀ¸·Î ¼º°øÀûÀ¸·Î ¿Å°ÜÁ³½À´Ï´Ù."

    "SUN"   = "ÀÏ"
    "MON"   = "¿ù"
    "TUE"   = "È­"
    "WED"   = "¼ö"
    "THU"   = "¸ñ"
    "FRI"   = "±Ý"
    "SAT"   = "Åä"

    "Not enough memory" = "¸Þ¸ð¸® ºÎÁ·"
    "%d notify entry is created.\n" =
        "%d °³ÀÇ ¾Ë¸² ·¹ÄÚµå°¡ »ý¼ºµÇ¾ú½À´Ï´Ù.\n"
    "%d existing entry " = "%d °³ÀÇ Á¸ÀçÇÏ´Â ·¹ÄÚµå"
    "is kept untouched." = "¸¦ ±×´ë·Î À¯ÁöÇß½À´Ï´Ù."
    "is modified." = "°¡ ¼öÁ¤µÇ¾ú½À´Ï´Ù."

    "Could not write prefs." = ""\
        "¼³Á¤È­ÀÏ ¾²±â ¿À·ù"
    "Collecting Information..." = ""\
        "ÁÖ¼Ò·ÏÀ¸·ÎºÎÅÍ Á¤º¸ ¼öÁýÁß..."

    "HappyDays List" = "HappyDays ¸ñ·Ï"
    "Look up" = "Ã£¾Æº¸±â"
    "Today" = "¿À´Ã"

    "HappyDays View" = "HappyDays º¸±â"
    "Done"  = "¿Ï·á"
    "DB" = "DB"
    "TD" = "TD"
    "Goto" = "ÀÌµ¿"

    "Utilities" = "À¯Æ¿¸®Æ¼ "
    "DateBook Notify" = "ÀÏÁ¤°ü¸®¿¡ ¾Ë¸² "
    "Cleanup Datebook"  = "ÀÏÁ¤°ü¸®¿¡¼­ Á¤¸® "
	"To Do Notify" = "ÇÒ ÀÏ¿¡ ¾Ë¸² "
	"Cleanup To Do"  = "ÇÒ ÀÏ¿¡¼­ Á¤¸® "
    "Export to Memo Pad" = "¸Þ¸ðÀåÀ¸·Î Àü¼Û "
    "Solar to Lunar" = "¾ç·ÂÀ» À½·ÂÀ¸·Î "
    "Lunar to Solar" = "À½·ÂÀ» ¾ç·ÂÀ¸·Î "

    "Options" = "¿É¼Ç "
    "Preferences" = "¼³Á¤ "
    "About HappyDays" = "HappyDays¶õ? "

    "Edit" = "¼öÁ¤"
    "Undo" = "µÇµ¹¸®±â"
    "Cut" = "ÀÚ¸£±â"
    "Copy" = "º¹»ç"
    "Paste" = "ºÙÀÌ±â"
    "Select All" = "ÀüÃ¼ ¼±ÅÃ"
    "Keyboard" = "Å°º¸µå"
    "Graffiti" = "±×·¡ÇÇÆ¼"

    "HappyDays Preferences" = "HappyDays ¼³Á¤"
    "Custom field:" = "»ç¿ëÀÚÁ¤ÀÇ ÇÊµå :"
    "HappyDays ID:" = "HD ½Äº°ÀÚ :"
    "Emphasize Lunar Date" = "À½·Â°­Á¶"

    "OK" = "½ÂÀÎ"
    "Cancel" = "Ãë¼Ò"

    "Solar to Lunar Convert" = "¾ç·ÂÀ» À½·ÂÀ¸·Î º¯È¯"
    "Enter Solar date:" = "¾ç·Â ³¯Â¥´Â :"
    "Year must be in 1881-2043" = "1881³â¿¡¼­ 2043³â±îÁö¸¸ Áö¿ø"

    "Result Lunar date:" = "º¯È¯µÈ À½·Â ³¯Â¥ :"

    "Convert" = "º¯È¯"

    "Lunar to Solar Convert" = "À½·ÂÀ» ¾ç·ÂÀ¸·Î º¯È¯"
    "Enter Lunar date:" = "À½·Â ³¯Â¥´Â :"
    "Leap" = "À±´Þ"

    "Result Solar date:" = "º¯È¯µÈ ¾ç·Â ³¯Â¥ :"

    "Records:" = "·¹ÄÚµå :"
      "all" = "ÀüºÎ"
      "selected" = "¼±ÅÃ"
    "Existing:" = "±âÁ¸ ·¹ÄÚµå :"
      "keep" = "À¯Áö"
      "modify" = "¼öÁ¤"
	"Priority:" = "¿ì¼±¼øÀ§ :"
	"Category:" = "Ä«Å×°í¸® :"
    "Private:" = "°³ÀÎ¿ë :"
    "Alarm:" = "¾Ë¶÷ :"
    "Time:" = "½Ã°£ :"
    "More..." = "´õ..."

    "Repeat for:" = "¸î ³â°£ :"
    "years" = "³â"

    "HappyDays says" = "Á¤ º¸"

    "HappyDays Export" = "HappyDays Àü¼Û"
    "Would you like to export HappyDays List to Memo Pad?" = ""\
        "¸Þ¸ðÀåÀ¸·Î HappyDays ¸ñ·ÏÀ» ¿Å±æ±î¿ä?"

    "Custom Field Error" = "»ç¿ëÀÚÁ¤ÀÇ ÇÊµå ¿¡·¯"
    "Help" = "µµ¿ò¸»"


    "HappyDays Cleanup" = "HappyDays Á¤¸®"

    "Notify warning" = "¾Ë¸² °æ°í"
	"^1: Are you sure you want to create notify entries for ^2 HappyDays records?" = ""\
	"^1: ^2 °³ÀÇ HappyDays¿¡ ´ëÇÑ ¾Ë¸² ·¹ÄÚµå¸¦ ¸¸µé±â ¿øÇÏ½Ê´Ï±î?"

    "Invalid event format" = "Àß¸øµÈ ÀÌº¥Æ® Æ÷¸Ë"

    "The address record for ^1\n contains an invalid event. How would you like to handle it?" = ""\
    "ÁÖ¼Ò·Ï ·¹ÄÚµå ^1\nÀÇ Æ÷¸ËÀÌ Àß¸øµÇ¾î ÀÖ½À´Ï´Ù. ¾î¶»°Ô Ã³¸®ÇÒ±î¿ä?"

    "Ignore" = "¹«½Ã"
    "Ignore all" = "ÀüºÎ ¹«½Ã"

    "Cleanup Done" = "Á¤¸®ÀÛ¾÷ ¿Ï·á"

	"^1: ^2 HappyDays notify entries removed." = ""\
	"^1: ^2 °³ÀÇ HappyDays ¾Ë¸² ·¹ÄÚµå°¡ Á¦°ÅµÇ¾ú½À´Ï´Ù."

	"(c) 1999-2000, JaeMok Jeong" = "(c) 1999-2000, Á¤ Àç¸ñ"
    "Licensed under the GNU GPL" = "GNU GPL ¶óÀÌ¼¾½º¿¡ µû¸¨´Ï´Ù"

    "(not exist in this year)" = "(¿ÃÇØ¿£ ¾ø´Â ³¯ÀÔ´Ï´Ù)"
    "Please run Date Book once first." = "¸ÕÀú ÀÏÁ¤°ü¸®¸¦ ÇÑ¹ø ¼öÇàÇÏ¼¼¿ä."

    "Instead of system date format" = "½Ã½ºÅÛ ³¯Â¥ Çü½Ä ´ë½Å¿¡"
    "Use" = "»ç¿ëÇÒ Çü½Ä"
    "M/D/Y" = "M/D/Y"
    "D/M/Y" = "D/M/Y"
    "D.M.Y" = "D.M.Y"
    "D-M-Y" = "D-M-Y"
    "Y/M/D" = "Y/M/D"
    "Y.M.D" = "Y.M.D"
    "Y-M-D" = "Y-M-D"

	"DateBook Notify More" = "¾ÆÀÌÄÜ ¼³Á¤"
	"Icon Support" = "¾ÆÀÌÄÜ Áö¿ø"
	"AN ICON Number:" = "AN ¾ÆÀÌÄÜ ¹øÈ£: "
	"DateBk3 ICON:" = "DateBk3 ¾ÆÀÌÄÜ: "

    "Days" = "ÀÏÀü"
	"Datebook" = "ÀÏÁ¤°ü¸®"
	"To Do" = "ÇÒ ÀÏ"
	"Scan note field" = "ÁÖ¼Ò·ÏÀÇ ³ëÆ® ÇÊµå ÀÐ±â"
	
	"Notify Format:" = "¾Ë¸² ·¹ÄÚµå Çü½Ä: "
	"[Last, First] E(1) Year" = "[¼º, ÀÌ¸§] E(1) ³ªÀÌ"
	"[First Last] E(1) Year" = "[ÀÌ¸§ ¼º] E(1) ³ªÀÌ"
	"Event - Last, First Year" = "Çà»ç - ¼º, ÀÌ¸§ ³ªÀÌ"
	"Event - First Last Year" = "Çà»ç - ÀÌ¸§ ¼º ³ªÀÌ"
	"First Last Year" = "ÀÌ¸§ ¼º ³ªÀÌ"

	"Processing" = "ÁøÇàÁß"
	"Processed : " = "Ã³¸®µÈ°Í : "
END


TRANSLATION "SPANISH"
BEGIN
    "Translator" = "Traducción: Fernando Calderón"
    "Translator's email" = "jfcalderon@usa.net"

    "long_notifySettingString" = ""\
      "* Registros\n"\
      "Seleccione \"Todos\" para crear notificaciones de "\
      "todos los eventos en esta categoría. "\
      "Selecione \"Marcado\" para notificar solamente el registo "\
      "actual.\n\n"\
      "* Existente\n"\
      "Seleccione \"dejar\" si no desea alterar las notificaciones existentes. "\
      "Seleccione \"cambiar\" para modificar estas entradas.\n\n"\
      "* Alarma\n"\
      "Para colocar la hora de alarma en la notificación. Indique "\
      "el número de días de anticipación para la notificación.\n\n"\
      "* Hora\n"\
      "Hora del evento\n\n"\
      "* Repetir\n"\
      "Establece el intervalo de la notificación. "\
      "Utilice -1 para repetir siempre en eventos solares y "\
      "repetir por 5 años en eventos lunares.\n\n"

    "long_notifySettingMoreString" = ""\
      "Para el Número de Icono de Action Names consulte su manual. "\
      "Los iconos de DateBk3 son leidos del bloc de notas. "\
      "Si no existe la entrada de DateBk3 en el bloc de notas no se muestra información.\n"

    "long_customErrorString" = ""\
    "'Nacimiento' es el nombre personalizado por omisión."\
    "Para establecer un campo personalizado diferente"\
    " seleccione 'Preferencias'.\n\n"\
    "IMPORTANTE: Necesitará utilizar Direcciones y seleccionar 'Opciones/"\
    "Nombre campos pers...' para cambiar un campo personalizado a 'Nacimiento'"\
    " o lo que desee establecer como nombre en las Preferencias de "\
    "HappyDays.\n"

    "long_aboutString" = ""\
      "'HappyDays' le ayudará a recordar los cumpleaños y aniversarios "\
      "de sus amigos y familiares, así como otros eventos.\n\n"\
      "Use uno de los campos personalizados de la Agenda y renombrelo como 'Nacimiento'."\
      "\n\nEjemplos:\n"\
      " Apellidos: Jeong\n"\
      " Nombre: JaeMok\n"\
      " ...\n"\
      " Nacimiento: -)12/4/1970\n\n"\
      "Formatos:\n"\
      " [-)|#)]Fecha\n"\
      " -) calendario lunar\n"\
      " #) calendario lunar bisiesto\n\n"\
      "Se utiliza el formato de fecha de las preferencias del sistema. "\
      "El año puede omitirse. Si escribe un año menor a 100 se asume del siglo 20. "\
      "Si no está seguro del formato de año, escribalo en la forma AAAA (como 1970).\n\n"\
      "Para entradas múltiples consulte el manual.\n"

    "long_cleanupAlertString" = ""\
    "Las notificaciones se consideran HappyDays si tienen "\
	"{HappyDays ID} en el campo Nota. "\ 	
    "Si usa este formato en sus propias entradas, "\
    "este registro puede ser removido por la Limpieza.\n"

    "long_prefFormString" = ""\
	"* En lugar de fecha del sistema...\n"\
	"Controla la interpretación del campo personalizado de Nacimiento. "\
	"Si se omite, es el mismo que las preferencias de fecha del sistema.\n\n"\
	"* Buscar en Notas\n"\
	"Para buscar en el campo de notas, seleccione esta opción.\n\n"\
      "* Enfatizar Fecha Lunar \n"\
      "Si se selecciona, la linea punteada se dibuja junto a la fecha lunar.\n\n"\
	"* Notify format\n"\
	"The description format used for notification\n\n"\
      "* Custom field\n"\
      "The name of custom fields in Address Book used for INPUT\n\n"\
      "* HappyDays ID\n"\
      "Used for Notify and Cleanup\n"


    "long_cantFindAddrString" = ""\
      "Kein Benutzerfeld '%s' im Adreßbuch gefunden.\n"\
      "Bitte ein Benutzerfeld umbenennen."

    "Select Time" = "Establecer hora"
    "No Time" = "Sin hora"
    "Invalid input date" = "Fecha inválida"
    "Export to Memo Pad is completed." = "Exportación a Bloc de notas lista."

    "SUN"   = "DOM"
    "MON"   = "LUN"
    "TUE"   = "MAR"
    "WED"   = "MIE"
    "THU"   = "JUE"
    "FRI"   = "VIE"
    "SAT"   = "SAB"

    "Not enough memory" = "Memoria Insuficiente"
    "%d notify entry is created.\n" = "%d notificación creada.\n"
    "%d existing entry " = "%d notificación existente "
    "is kept untouched." = "sin modificar."
    "is modified." = "modificada."

    "Could not write prefs." = "No se ha podido guardar preferencias."
    "Collecting Information..." = "Recolectando información..."

    "HappyDays List" = "Lista HappyDays"
    "Look up" = "Buscar"
    "Today" = "Hoy"

	"HappyDays View" = "Detalles HappyDays"
    "Done"  = "Hecho"
    "DB" = "Agenda"
    "TD" = "Tareas"
    "Goto" = "Ir a"

    "Utilities" = "Utilerias"
    "DateBook Notify" = "Notificar en Agenda"
    "Cleanup Datebook"  = "Limpiar Agenda"
    "To Do Notify" = "Notificar en Tareas"
    "Cleanup To Do"  = "Limpiar Tareas"
    "Export to Memo Pad" = "Exportar a Bloc de notas"
    "Solar to Lunar" = "Solar -> Lunar"
    "Lunar to Solar" = "Lunar -> Solar"

    "Options" = "Opciones"
    "Preferences" = "Preferencias"
    "About HappyDays" = "Acerca de HappyDays"

    "Edit" = "Edición"
    "Undo" = "Deshacer"
    "Cut" = "Cortar"
    "Copy" = "Copiar"
    "Paste" = "Pegar"
    "Select All" = "Selec todo"
    "Keyboard" = "Teclado"
    "Graffiti" = "Ayuda de Graffiti"

    "HappyDays Preferences" = "Preferencias de HappyDays"
    "Custom field:" = "Campo pers:"
    "HappyDays ID:" = "Identif. HappyDays:"
    "Emphasize Lunar Date" = "Preferir Calendario Lunar"

    "OK" = "Hecho"
    "Cancel" = "Cancelar"

    "Solar to Lunar Convert" = "Solar -> Lunar"
    "Enter Solar date:" = "Indique la fecha Solar:"
    "Year must be in 1881-2043" = "El Año debe ser entre 1881 y 2043"
    "Result Lunar date:" = "Fecha Lunar resultado:"

    "Convert" = "Convertir"

    "Lunar to Solar Convert" = "Lunar -> Solar."
    "Enter Lunar date:" = "Indique la fecha Lunar:"
    "Leap" = "Bisiesto"
    "Result Solar date:" = "Fecha Solar resultado:"

    "Records:" = "Registros:"
      "all" = "Todos"
      "selected" = "Marcado"
    "Existing:" = "Existente:"
      "keep" = "dejar"
      "modify" = "cambiar"
    "Priority:" = "Prioridad:"
	"Category:" = "Categoría:"
    "Private:" = "Confidencial:"
    "Alarm:" = "Alarma:"
    "Time:" = "Hora:"
    "More..." = "Más..."

    "Repeat for:" = "Repetir:"
    "years" = "año(s)"

    "HappyDays says" = "HappyDays dice"

    "HappyDays Export" = "Exportar HappyDays"
    "Would you like to export HappyDays List to Memo Pad?" = ""\
      "¿Desea exportar la lista HappyDays al Bloc de notas?"

    "Custom Field Error" = "Error en campo personalizado"
    "Help" = "Ayuda"


    "HappyDays Cleanup" = "Limpiar HappyDays"

    "Notify warning" = "Aviso"
    "^1: Are you sure you want to create notify entries for ^2 HappyDays records?" = ""\
      "^1: ¿Está seguro que desea crear notificaciones de HappyDays para ^2 registros?"

    "Invalid event format" = "Formato de evento invalido."

    "The address record for ^1\n contains an invalid event. How would you like to handle it?" = ""\
      "El registro de agenda de ^1\n contiene un evento invalido. ¿Cómo desea proceder?"

    "Ignore" = "Ignorar"
    "Ignore all" = "Ignorar todos"

    "Cleanup Done" = "Limpieza concluida"

    "^1: ^2 HappyDays notify entries removed." = ""\
      "^1: ^2 notificaciones de HappyDays eliminadas."

    "Birthday" = "Nacimiento"
    "All" = "Todos"
    "HappyDays Export List" = "Exportar Lista HappyDays"
    "(not exist in this year)" = "(no existe en este año)"
    "Please run Date Book once first." = "Por favor selecione la Agenda primero."
    
    "Instead of system date format" = "Reemplazar el formato de fecha del sistema"
    "Use" = "Usar"
    "M/D/Y" = "M/D/A"
    "D/M/Y" = "D/M/A"
    "D.M.Y" = "D.M.A"
    "D-M-Y" = "D-M-A"
    "Y/M/D" = "A/M/D"
    "Y.M.D" = "A.M.D"
    "Y-M-D" = "A-M-D"

	"DateBook Notify More" = "Más notificación de Agenda"
      "Icon Support" = "Soporte de Iconos"
	  "AN" = "AN"
	  "DateBk3" = "DBk3"
      "AN ICON Number:" = "Número Icono AN:"
      "DateBk3 ICON:" = "Icono DateBk3:"

    "Days" = "Días"
	"Datebook" = "Agenda"
	"To Do" = "Tareas"
	"Scan note field" = "Buscar en Notas"
	
	"Notify Format:" = "Formato de Notificación:"
    "[Last, First] E(1) Year" = "[Apellido, Nombre] E(1) Año"
	"[First Last] E(1) Year" = "[Nombre Apellido] E(1) Año"
	"Event - Last, First Year" = "Evento - Apellido, Nombre Año"
	"Event - First Last Year" = "Evento - Nombre Apellido Año"
	"First Last Year" = "Nombre Apellido Año"
	
	"Processing" = "Procesando"
	"Processed : " = "Procesado : "	
END


STRING ID DateBookNotifyHelpString "long_notifySettingString"
STRING ID DateBookNotifyMoreHelpString "long_notifySettingMoreString"
STRING ID SelectDateString  "HappyDays"
STRING ID CustomErrorHelpString "long_customErrorString"
STRING ID AboutHelpString "long_aboutString"
STRING ID CleanupAlertHelpString "long_cleanupAlertString"
STRING ID PrefFormHelpString "long_prefFormString"
STRING ID RemoveConfirmString "long_removeConfirmString"
STRING ID InvalidDateString "Invalid input date"
STRING ID SelectTimeString  "Select Time"
STRING ID NoTimeString      "No Time"
STRING ID ExportDoneString  "Export to Memo Pad is completed."
STRING ID Sl2LnFormHelpString "long_sl2lnString"

STRING ID SunString     "SUN"
STRING ID MonString     "MON" 
STRING ID TueString     "TUE"
STRING ID WedString     "WED"
STRING ID ThuString     "THU"
STRING ID FriString     "FRI"
STRING ID SatString     "SAT"

STRING ID NotEnoughMemoryString
                "Not enough memory"
STRING ID NotifyCreateString
                "%d notify entry is created.\n"
STRING ID ExistingEntryString   "%d existing entry "
STRING ID UntouchedString       "is kept untouched."
STRING ID ModifiedString        "is modified."
STRING ID CantFindAddrString    "long_cantFindAddrString"
STRING ID CantWritePrefsString  "Could not write prefs."
STRING ID ExportHeaderString 	"HappyDays Export List"
STRING ID ViewNotExistString	"(not exist in this year)"
STRING ID DateBookFirstAlertString  "Please run Date Book once first."
STRING ID DateBookNotifyDaysString "Days"

STRING ID DateBookString		"Datebook"
STRING ID ToDoString			"To Do"

FORM StartForm AT (0 0 160 160)
NOFRAME
SAVEBEHIND
BEGIN
    LABEL "Collecting Information..." AUTOID AT (Center@80 BOTTOM@125) FONT 1
END

FORM MainForm AT (0 0 160 160)
USABLE
NOFRAME
SAVEBEHIND
MENUID MainFormMenu
BEGIN
    TITLE "HappyDays"
    TABLE ID MainFormTable AT (0 16 160 121) ROWS 11 COLUMNS 1
                COLUMNWIDTHS 160
    BUTTON "Look up" ID MainFormLookup AT (1 BOTTOM@160-1 AUTO AUTO)
    FIELD  ID MainFormField AT (PrevRight+3 PrevTop 30 AUTO) UNDERLINED
            NONEDITABLE MAXCHARS 5
    BUTTON "Today" ID MainFormToday AT (PrevRight+2 PrevTop AUTO AUTO)
    POPUPTRIGGER "" ID MainFormPopupTrigger
            AT ( RIGHT@159 1 AUTO AUTO ) RIGHTANCHOR 
    LIST "" ID MainFormAddrCategories AT (PREVLEFT PREVTOP 70 AUTO)
            NONUSABLE
    POPUPLIST ID MainFormPopupTrigger MainFormAddrCategories

    BUTTON " " ID MainFormPageUp AT ( RIGHT@159 141 13 8 )
        NONUSABLE NOFRAME FONT 5
    BUTTON " " ID MainFormPageDown AT ( PREVLEFT PREVBOTTOM 13 8 )
        NONUSABLE NOFRAME FONT 5
END

FORM BirthdateForm AT (0 0 160 160)
USABLE
NOFRAME
SAVEBEHIND
MENUID ViewFormMenu
BEGIN
    TITLE "HappyDays View"

	FIELD ID BirthdateCategory AT (RIGHT@158 1 65 AUTO) RIGHTALIGN 
		NONEDITABLE FONT 0
    FIELD ID BirthdateFirst AT (6 30 148 AUTO) NONEDITABLE FONT 2
    FIELD ID BirthdateSecond AT (PrevLeft PrevBottom+5 148 AUTO) NONEDITABLE
        FONT 2
    FIELD ID BirthdateCustom AT (PrevLeft PrevBottom+8 148 AUTO)
        NONEDITABLE FONT 1
    FIELD ID BirthdateDate AT (PrevLeft PrevBottom+3 148 AUTO)
        NONEDITABLE FONT 2
    FIELD ID BirthdateOrigin AT (PrevLeft PrevBottom+3 148 AUTO)
        NONEDITABLE FONT 0

    BUTTON "Done" ID BirthdateDone AT (1 BOTTOM@159 AUTO AUTO)
    BUTTON "DB" ID BirthdateNotifyDB AT (PrevRight+6 PrevTop AUTO AUTO)
    BUTTON "TD" ID BirthdateNotifyTD AT (PrevRight+1 PrevTop AUTO AUTO)
    BUTTON "Goto" ID BirthdateGoto AT ( PrevRight+6 PrevTop AUTO AUTO)
END

MENU ID MainFormMenu
BEGIN
    PULLDOWN "Utilities"
    BEGIN
        MENUITEM "DateBook Notify"   	MainFormMenuNotifyDatebook  "D"
        MENUITEM "Cleanup Datebook"     MainFormMenuCleanupDatebook "C"
        MENUITEM "To Do Notify"   		MainFormMenuNotifyTodo		"T"
        MENUITEM "Cleanup To Do"       	MainFormMenuCleanupTodo 	"O"
        MENUITEM SEPARATOR
        MENUITEM "Export to Memo Pad"   MainFormMenuExport  		"E" 
        MENUITEM SEPARATOR
        MENUITEM "Solar to Lunar"       MainFormMenuSl2Ln   		"S"
        MENUITEM "Lunar to Solar"       MainFormMenuLn2Sl   		"L"
    END

    PULLDOWN "Options"
    BEGIN
        MENUITEM "Preferences"              MainFormMenuPref    "R"
        MENUITEM SEPARATOR
        MENUITEM "About HappyDays"          MainFormMenuAbout   "A"
    END
END

MENU ID ViewFormMenu
BEGIN
    PULLDOWN "Utilities"
    BEGIN
        MENUITEM "Solar to Lunar"           MainFormMenuSl2Ln   "S"
        MENUITEM "Lunar to Solar"           MainFormMenuLn2Sl   "L"
    END
END

// Edit menu for S2L, L2S
MENU ID TextMenu
BEGIN
    PULLDOWN   "Edit"
    BEGIN
    MENUITEM "Undo"                         TextMenuUndo        "U"
    MENUITEM "Cut"                          TextMenuCut         "X"
    MENUITEM "Copy"                         TextMenuCopy        "C"
    MENUITEM "Paste"                        TextMenuPaste       "P"
    MENUITEM "Select All"                   TextMenuSAll        "S"
    MENUITEM  SEPARATOR
    MENUITEM "Keyboard"                     TextMenuKBoard      "K"
    MENUITEM "Graffiti"                     TextMenuGHelp       "G"
    END
END

FORM ID PrefForm AT (2 2 156 156)
FRAME
MODAL
USABLE
DEFAULTBTNID PrefFormCancel
HELPID PrefFormHelpString
BEGIN
    TITLE "HappyDays Preferences"
    CHECKBOX "Instead of system date format" ID PrefFormOverrideSystemDate AT 
			(10 20 AUTO AUTO)
    LABEL "Use" AUTOID AT (20 PREVBOTTOM)
    LIST "M/D/Y" "D/M/Y" "D.M.Y" "D-M-Y" "Y/M/D" "Y.M.D" "Y-M-D" 
		ID PrefFormDateFmts 
		AT (PREVRIGHT+4 PREVTOP 40 AUTO) VISIBLEITEMS 7 NONUSABLE
    POPUPTRIGGER "" ID PrefFormDateTrigger AT (PREVLEFT PREVTOP-1 40 13) LEFTANCHOR
    POPUPLIST ID PrefFormDateTrigger PrefFormDateFmts

	CHECKBOX "Scan note field" ID PrefFormScanNote
		AT (10 PrevBottom AUTO AUTO)
    CHECKBOX "Emphasize Lunar Date" ID PrefFormEmphasize 
		AT (10 PrevBottom AUTO AUTO) 

	LABEL "Notify Format:" AUTOID AT (10 PrevBottom+5) 
    LIST 	"[Last, First] E(1) Year" 
	    	"[First Last] E(1) Year" 
			"Event - Last, First Year"
			"Event - First Last Year"
			"First Last Year"
		ID PrefFormNotifyFmts 
			AT (20 PrevBottom 120 AUTO) VISIBLEITEMS 5 NONUSABLE
    POPUPTRIGGER "" ID PrefFormNotifyTrigger 
			AT (PREVLEFT PREVTOP-1 40 13) LEFTANCHOR
    POPUPLIST ID PrefFormNotifyTrigger PrefFormNotifyFmts

    LABEL "Custom field:" AUTOID AT (10 PrevBottom+5) 
    FIELD ID PrefFormCustomField AT (85 PrevTop 50 AUTO) UNDERLINED
            MAXCHARS 12
    LABEL "HappyDays ID:" AUTOID AT (10 PrevBottom+1) 
    FIELD ID PrefFormNotifyWith AT (85 PrevTop 28 AUTO) UNDERLINED
            MAXCHARS 5

    BUTTON "OK" ID PrefFormOk AT (5 BOTTOM@149 40 AUTO)
    BUTTON "Cancel" ID PrefFormCancel AT (PrevRight+5 PrevTop 40 AUTO)
	GRAFFITISTATEINDICATOR AT (PrevRight+5 PrevTop)
END

FORM ID Sl2LnForm AT (1 2 158 156)
USABLE
SAVEBEHIND
MODAL
HELPID Sl2LnFormHelpString
MENUID TextMenu
BEGIN
    TITLE "Solar to Lunar Convert"
    LABEL "Enter Solar date:" AUTOID AT (10 30) FONT 1
    FIELD ID Sl2LnFormInput AT (10 PrevBottom+3 70 AUTO) MAXCHARS 10
        UNDERLINED
    LABEL "Year must be in 1881-2043" AUTOID AT (10 PrevBottom+3) FONT 0

    LABEL "Result Lunar date:" AUTOID AT (10 PrevBottom+15) FONT 1
    FIELD ID Sl2LnFormResult AT (10 PrevBottom+3 70 AUTO)
        NONEDITABLE UNDERLINED MAXCHARS 15

    BUTTON "Convert" ID Sl2LnFormConvert AT
            (10 BOTTOM@151 40 AUTO)
    BUTTON "OK" ID Sl2LnFormOk AT (PrevRight+5 PrevTop 40 AUTO)

END

FORM ID Ln2SlForm AT (1 2 158 156)
USABLE
SAVEBEHIND
MODAL
MENUID TextMenu
BEGIN
    TITLE "Lunar to Solar Convert"
    LABEL "Enter Lunar date:" AUTOID AT (10 30) FONT 1
    FIELD ID Ln2SlFormInput AT (10 PrevBottom+3 70 AUTO) MAXCHARS 10
        UNDERLINED
    CHECKBOX "Leap" ID Ln2SlFormInputLeap AT
        (PrevRight+8 PrevTop AUTO AUTO) LEFTANCHOR FONT 0
    LABEL "Year must be in 1881-2043" AUTOID AT (10 PrevBottom+3) FONT 0

    LABEL "Result Solar date:" AUTOID AT (10 PrevBottom+15) FONT 1
    FIELD ID Ln2SlFormResult AT (10 PrevBottom+3 80 AUTO)
        NONEDITABLE UNDERLINED MAXCHARS 15

    BUTTON "Convert" ID Ln2SlFormConvert AT
            (10 BOTTOM@151 40 AUTO)
    BUTTON "OK" ID Ln2SlFormOk AT (PrevRight+5 PrevTop 40 AUTO)

END

FORM ID DateBookNotifyForm AT (2 18 156 140)
FRAME
MODAL
USABLE
DEFAULTBTNID DateBookNotifyFormCancel
HELPID  DateBookNotifyHelpString
BEGIN
    TITLE "DateBook Notify"
    LABEL "Records:" AUTOID AT (RIGHT@78 25) FONT 1
    PUSHBUTTON "all" ID NotifyFormRecordAll 
			AT (PrevRight+5 PrevTop AUTO AUTO) GROUP 1
    PUSHBUTTON "selected" ID NotifyFormRecordSlct 
			AT (PrevRight+1 PrevTop AUTO AUTO) LEFTANCHOR GROUP 1

    LABEL "Existing:" AUTOID AT (RIGHT@78 PrevBottom+3) FONT 1
    PUSHBUTTON "keep" ID NotifyFormEntryKeep 
			at (PrevRight+5 PrevTop AUTO AUTO) GROUP 2
    PUSHBUTTON "modify" ID NotifyFormEntryModify 
			at (PrevRight+1 PrevTop AUTO AUTO) LEFTANCHOR GROUP 2

    LABEL "Private:" AUTOID AT (RIGHT@78 PrevBottom+3) FONT 1
    CHECKBOX " " ID NotifyFormPrivate 
			at (PrevRIght+5 PrevTop AUTO AUTO) RIGHTANCHOR FONT 1 

    LABEL "Alarm:" AUTOID AT (RIGHT@78 PrevBottom+3) FONT 1
    CHECKBOX "" ID DateBookNotifyFormAlarm at (PrevRight+5 PrevTop
            AUTO AUTO) RIGHTANCHOR FONT 1
    FIELD ID DateBookNotifyFormBefore AT (PrevRight+2 PrevTop 13 AUTO)
        UNDERLINED MAXCHARS 2
    FIELD  ID DateBookNotifyFormLabelDays AT (PrevRight PrevTop 30 AUTO) 
		NONEDITABLE 

    LABEL "Time:" AUTOID AT (RIGHT@78 PrevBottom+3) FONT 1
    SELECTORTRIGGER "" ID DateBookNotifyFormTime at (PrevRight+5 PrevTop
            AUTO AUTO) LEFTANCHOR FONT 0

    LABEL "Repeat for:" AUTOID AT (RIGHT@78 PrevBottom+3) FONT 1
    FIELD ID DateBookNotifyFormDuration AT (PrevRight+5 PrevTop 13 AUTO)
            UNDERLINED MAXCHARS 2
    LABEL "years" AUTOID AT (PrevRight+3 PrevTop) FONT 0

    BUTTON "OK" ID DateBookNotifyFormOk AT (5 Bottom@135 40 AUTO)
    BUTTON "Cancel" ID DateBookNotifyFormCancel AT (PrevRight+5 PrevTop 40 AUTO)
	BUTTON "More..." ID DateBookNotifyFormMore AT (RIGHT@150 PrevTop AUTO AUTO)
END

FORM ID DateBookNotifyMoreForm AT (2 18 156 140)
FRAME
MODAL
USABLE
HELPID  DateBookNotifyMoreHelpString
BEGIN
    TITLE "DateBook Notify More"

    CHECKBOX "Icon Support" ID DateBookNotifyFormIcon 
			at (7 25 AUTO AUTO) RIGHTANCHOR FONT 0 
    PUSHBUTTON "AN" ID DateBookNotifyFormAN AT (PrevRight+5
            PrevTop AUTO AUTO) LEFTANCHOR GROUP 1
    PUSHBUTTON "DateBk3" ID DateBookNotifyFormBk3 AT (PrevRight+1
            PrevTop AUTO AUTO) GROUP 1

	LABEL "AN ICON Number:" ID DateBookNotifyANLabel AT (5 PrevBottom+5) 
	FIELD ID DateBookNotifyANInput AT (PrevRight+5 PrevTop 30 AUTO) 
		UNDERLINED MAXCHARS 3

	LABEL "DateBk3 ICON:" ID DateBookNotifyBk3Label AT (5 PrevBottom+2) 
    TABLE ID DateBookNotifyBk3Table AT (7 PrevBottom 143 40) ROWS 4 
		COLUMNS 13 COLUMNWIDTHS 10 10 10 10 10 10 10 10 10 10 10 10 10

    BUTTON "OK" ID DateBookNotifyMoreFormOk AT (5 Bottom@135 40 AUTO)
	GRAFFITISTATEINDICATOR AT (PrevRight+5 PrevTop)
END

FORM ID ToDoNotifyForm AT (2 28 156 130)
FRAME
MODAL
USABLE
DEFAULTBTNID ToDoNotifyFormCancel
BEGIN
	TITLE "To Do Notify"

    LABEL "Records:" AUTOID AT (RIGHT@65 20) FONT 1
    PUSHBUTTON "all" ID NotifyFormRecordAll 
			AT (PrevRight+5 PrevTop AUTO AUTO) GROUP 1
    PUSHBUTTON "selected" ID NotifyFormRecordSlct 
			AT (PrevRight+1 PrevTop AUTO AUTO) LEFTANCHOR GROUP 1

    LABEL "Existing:" AUTOID AT (RIGHT@65 PrevBottom+5) FONT 1
    PUSHBUTTON "keep" ID NotifyFormEntryKeep 
			at (PrevRight+5 PrevTop AUTO AUTO) GROUP 2
    PUSHBUTTON "modify" ID NotifyFormEntryModify 
			at (PrevRight+1 PrevTop AUTO AUTO) LEFTANCHOR GROUP 2

	LABEL "Priority:" AUTOID AT (RIGHT@65 PrevBottom+5) FONT 1
	PUSHBUTTON "1" ID ToDoNotifyPri1 AT (PrevRight+5 PrevTop AUTO AUTO)
		LEFTANCHOR GROUP 3
	PUSHBUTTON "2" ID ToDoNotifyPri2 AT (PrevRight+1 PrevTop AUTO AUTO)
		LEFTANCHOR GROUP 3
	PUSHBUTTON "3" ID ToDoNotifyPri3 AT (PrevRight+1 PrevTop AUTO AUTO)
		LEFTANCHOR GROUP 3
	PUSHBUTTON "4" ID ToDoNotifyPri4 AT (PrevRight+1 PrevTop AUTO AUTO)
		LEFTANCHOR GROUP 3
	PUSHBUTTON "5" ID ToDoNotifyPri5 AT (PrevRight+1 PrevTop AUTO AUTO)
		LEFTANCHOR GROUP 3

	LABEL "Category:" AUTOID AT (RIGHT@65 PrevBottom+5) FONT 1
    POPUPTRIGGER "" ID ToDoPopupTrigger
            AT ( PrevRight+5 PrevTop AUTO AUTO ) LeftAnchor 
	LIST "" ID ToDoNotifyCategory AT ( PrevLeft PrevTop 70 AUTO ) 
		NONUSABLE 
    POPUPLIST ID ToDoPopupTrigger ToDoNotifyCategory
	
    LABEL "Private:" AUTOID AT (RIGHT@65 PrevBottom+5) FONT 1
    CHECKBOX " " ID NotifyFormPrivate 
			at (PrevRight+5 PrevTop AUTO AUTO) RIGHTANCHOR FONT 1 

    BUTTON "OK" ID ToDoNotifyFormOk AT (5 Bottom@125 40 AUTO)
    BUTTON "Cancel" ID ToDoNotifyFormCancel AT (PrevRight+5 PrevTop 40 AUTO)
END

ALERT ID ErrorAlert INFORMATION
BEGIN
    TITLE "HappyDays says"
    MESSAGE "^1"
    BUTTONS "OK"
END

ALERT ID ExportAlert CONFIRMATION
DEFAULTBUTTON 1
BEGIN
    TITLE "HappyDays Export"
    MESSAGE 
        "Would you like to export HappyDays List to Memo Pad?"
    BUTTONS "OK" "Cancel"
END

ALERT ID CustomFieldAlert INFORMATION
HELPID CustomErrorHelpString
BEGIN
    TITLE "Custom Field Error"
    MESSAGE "^1"
    BUTTONS "OK" "Help" "Preferences"
END

ALERT ID CleanupAlert CONFIRMATION
HELPID CleanupAlertHelpString
DEFAULTBUTTON 1
BEGIN
    TITLE "HappyDays Cleanup"
    MESSAGE "^1: ^2"
    BUTTONS "OK" "Cancel"
END

ALERT ID NotifyWarning CONFIRMATION
DEFAULTBUTTON 1
BEGIN
    TITLE "Notify warning"
    MESSAGE "^1: Are you sure you want to create notify entries for ^2 HappyDays records?"
    BUTTONS "OK" "Cancel"
END

ALERT ID InvalidFormat CONFIRMATION
BEGIN
    TITLE "Invalid event format"
    MESSAGE "The address record for ^1\n contains an invalid event. How would you like to handle it?"
    BUTTONS "Edit" "Ignore" "Ignore all"
END

ALERT ID CleanupDone INFORMATION
BEGIN
    TITLE "Cleanup Done"
    MESSAGE "^1: ^2 HappyDays notify entries removed."
    BUTTONS "OK"
END

FORM ID AboutForm AT (2 2 156 156)
USABLE
MODAL
HELPID AboutHelpString
BEGIN
    TITLE "HappyDays"
    LABEL "HappyDays 1.24" AUTOID AT (Center@80 20) FONT 1
    LABEL "(c) 1999-2000, JaeMok Jeong"
        AUTOID AT (Center@80 PrevBottom+5) FONT 0
    LABEL "<jmjeong@oopsla.snu.ac.kr>" AUTOID AT (Center@80 PrevBottom)
        Font 0
    LABEL "http://oopsla.snu.ac.kr/~jmjeong/"
        AUTOID AT (Center@80 PrevBottom) FONT 0
    LABEL "palm/happydays" AUTOID AT (PrevLeft+10 PrevBottom) FONT 0

    LABEL "Licensed under the GNU GPL" AUTOID AT (Center@80
        PrevBottom+4) FONT 0

    LABEL "Translator" AUTOID AT (Center@80 PrevBottom+3) USABLE FONT 0
    LABEL "Translator's email" AUTOID AT (Center@80 PrevBottom) USABLE FONT 0
            
    BUTTON "OK" AboutFormOk AT (RIGHT@80-3 BOTTOM@148 44 AUTO) FONT 0
    BUTTON "Help" AboutFormHelp AT (PrevRight+6 PrevTop 44 AUTO) FONT 0
END

FORM ID ProgressForm AT (22 72 116 46)
USABLE
MODAL
BEGIN
	TITLE "Processing"
	LABEL "Processed : " AUTOID AT (20 25) 
	FIELD ID ProgressFormField AT (PrevRight+3 PrevTop 50 AUTO) NONEDITABLE
END

APPLICATIONICONNAME 1000 "HappyDays"
ICON "happydays.pbitm"
SMALLICON "happydays-small.pbitm"

VERSION 1 "1.24"
