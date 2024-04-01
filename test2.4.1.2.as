 lw 0 1 five
 lw 0 2 nine
 lw 0 3 one
 nor 1 1 1
 add 3 1 1
 add 2 1 1
loop beq 1 4 done
 add 2 2 2
 add 4 3 4
 beq 0 0 loop
done halt
one	.fill	1
two	.fill	2
three	.fill	3
four	.fill	4
five	.fill	5
six	.fill	6
seven	.fill	7
eight	.fill	8
nine	.fill	9
