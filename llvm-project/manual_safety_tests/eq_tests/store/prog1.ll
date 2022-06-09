target datalayout = "e-m:e-p:32:32-p270:32:32-p271:32:32-p272:64:64-f64:32:64-f80:32-n8:16:32-S128"
target triple = "i386-unknown-linux-gnu"

@ptr = global i32 21

define void @main(){
  %i_1 = shl i32 0, 3333
  store i32 %i_1, i32* @ptr
  ret void
}
