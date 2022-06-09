target datalayout = "e-m:e-p:32:32-p270:32:32-p271:32:32-p272:64:64-f64:32:64-f80:32-n8:16:32-S128"
target triple = "i386-unknown-linux-gnu"

define i4 @func(){
  ret i4 undef
}

define i4 @main(){
        %i_0 = call i4 @func()
        %i_1 = call i4 @func()
	%i_2 = add i4 %i_0, %i_1
	ret i4 %i_2
}
