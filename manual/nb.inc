#use wml::des::navbar

<navbar:define name=jmjeong>
	<navbar:prolog pos=first>[ </navbar:prolog>
	<navbar:epilog>/</navbar:epilog>
	<navbar:epilog pos=last> ]</navbar:epilog>

	<navbar:button id=About txt="About" url="$(ROOT)/index.html">
	<navbar:button id=Download txt="Download" url="$(ROOT)/download.html">
	<navbar:button id=Manual txt="Manual" url="$(ROOT)/manual.html">
	<navbar:button id=FAQ txt="FAQ" url="$(ROOT)/faq.html">
	<navbar:button id=List txt="Mailing list" url="$(ROOT)/mailinglist.html">
</navbar:define>

<define-container H2>
<table width="100%" cellspacing="0" cellpadding="0" border="0">
<tr>
  <td bgcolor="#e0e0e0">
      <font color="#000000" face="Helvetica, Arial">
	  <b>
		%body
	  </b>
	  </font>
  </td>
</tr>
</table>
</define-container>

<define-container H1>
<protect><h2></protect>%body<protect></h2></protect>
</define-container>
