BEGIN {
	# determine input and output file name from given parameter
	inputfile = lang ".msg"
	outputfile = lang "-new.msg"

	# scan the input file for 
	while (getline l < inputfile)
	{
		if (cont == 0 && match(l, /^[ \t]*\"[^\"]+\"[ \t]*=[ \t]*/))
		{
			s = substr(l, RSTART, RLENGTH)
			sub(/^[ \t]*/, "", s)
			sub(/[ \t]*=[ \t]*$/, "", s)
			t = substr(l, RSTART+RLENGTH)
		}
		else if (cont == 1)
		{
			t = t "\n" l
		}
		
		if (l ~ /\\$/)
			cont = 1
		else
			cont = 0

		if (cont == 0 && s > "")
		{
			trans[s] = t
			s = ""
			t = ""
		}
	}
	close(inputfile)
	n = 0
	el = 0
}
{
	l = $0
	while (match(l, /\"[^\"]+\"/) > 0)
	{
		s = substr(l, RSTART, RLENGTH)
		l = substr(l, RSTART+RLENGTH)
		if ((length(s) > 1) && !(s in strings))
		{
			strlist[n++] = s
			strings[s] = 1
			el = 0
		}
	}
	if ($1 == "")
	{
		if (el == 0)
			n++
		el = 1
	}
}
END {
	print "TRANSLATION \"" toupper(lang) "\"\nBEGIN" > outputfile

	for (i=0; i<n; i++)
	{
		s = strlist[i]
		if (s > "")
		{
			printf "    %s", s > outputfile
			if (s in trans)
			{
				print " = ", trans[s] > outputfile
				delete trans[s]
			}
			else
			{
				print "= \"---NEW---\"" > outputfile
			}
		}
		else
		{
			print "" > outputfile
		}
	}
	print "END\n\n--------------------- removed ------------------------------" > outputfile
	for (i in trans)
		print i " = " trans[i] > outputfile
}
