#include wml::std::box
#include "nb.inc"

# The Template
<HTML>
<HEAD>
<meta name="description" content="HappyDays, Palm Pilot, application ">
<meta name="keywords" content="HappyDays, Palm Pilot, PDA">
<TITLE*>HappyDays-- <<TITLE>></TITLE*>
</HEAD>
<BODY bgcolor=white 
      MARGINWIDTH=0 MARGINHEIGHT=0>

# nav bar
<center>
<navbar:render name=jmjeong select=$(select)>
</center>

<br>
<br>

<<H1>>
<<Body>>
<HR noshade size=1>
<DIV align=right><FONT color="#bfbfbf">
<small>
This page was updated on, <:print "".localtime(time):></small></font></DIV>
# The End
</BODY>

# All the rest goes into Body
..Body>>

