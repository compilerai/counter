define void @main(){
	%i_1 = shl i4 3, 1
	%i_2 = icmp eq i4 %i_1, 0
	br i1 %i_2, label %foo, label %end
	foo:
		%i_3 = shl i4 3, %i_1
		br label %end
	end:
	ret void
}
