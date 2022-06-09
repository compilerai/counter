target datalayout = "e-m:e-p:32:32-p270:32:32-p271:32:32-p272:64:64-f64:32:64-f80:32-n8:16:32-S128"
target triple = "i386-unknown-linux-gnu"

define void @main(){
	%i_1 = shl i4 3, 5
	%i_2 = icmp eq i4 %i_1, 0
	br i1 %i_2, label %foo, label %end
	foo:
		%i_3 = shl i4 3, %i_1
		br label %end
	end:
	ret void
}
