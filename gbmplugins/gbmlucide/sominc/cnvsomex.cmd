/* 
 * Convert 'SOMEXTERN struct' to 'SOMEXTERNS struct'
 * (for gcc compatibility)
 *
 * Usage:
 *    cnvsomex <filename.xh>
 */

parse arg f
tmp = "tmp.tmp"

sstr = "SOMEXTERN struct"
sstrlen = LENGTH( sstr )

do while lines(f)
	l=linein(f)
	if SUBSTR(l,1,sstrlen)=sstr then do
		resstr = "SOMEXTERNS struct"||SUBSTR(l,sstrlen+1)
		l=resstr
	end
	call lineout tmp, l
end	

call lineout f
call lineout tmp

"@del "||f
"@ren "||tmp||" "||f
