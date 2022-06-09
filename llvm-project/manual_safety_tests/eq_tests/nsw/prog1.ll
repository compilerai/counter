target datalayout = "e-m:e-p:32:32-p270:32:32-p271:32:32-p272:64:64-f64:32:64-f80:32-n8:16:32-S128"
target triple = "i386-unknown-linux-gnu"

define void @main(){
	%i_2 = add nsw i4 10 , 12
	%i_3 = icmp eq i4 %i_2, 0
	br i1 %i_3, label %foo, label %end
	foo:
		%i_4 = shl i4 3, 2
		br label %end
	end:
	ret void
}
