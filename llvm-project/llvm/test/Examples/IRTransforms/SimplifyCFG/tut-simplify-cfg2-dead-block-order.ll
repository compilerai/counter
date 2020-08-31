; NOTE: Assertions have been autogenerated by utils/update_test_checks.py
; RUN: opt -tut-simplifycfg -tut-simplifycfg-version=v1 -S < %s | FileCheck %s
; RUN: opt -tut-simplifycfg -tut-simplifycfg-version=v2 -S < %s | FileCheck %s
; RUN: opt -tut-simplifycfg -tut-simplifycfg-version=v3 -S < %s | FileCheck %s

define i32 @remove_dead_blocks() {
; CHECK-LABEL: @remove_dead_blocks(
; CHECK-NEXT:  entry:
; CHECK-NEXT:    ret i32 1
; CHECK-NEXT:  }
;
entry:
  ret i32 1

bb.1:
  ret i32 2

bb.2:
  ret i32 3
}

define i32 @simp1() {
; CHECK-LABEL: @simp1(
; CHECK-NEXT:  entry:
; CHECK-NEXT:    ret i32 1
; CHECK:       bb.1:
; CHECK-NEXT:    ret i32 2
; CHECK-NEXT:  }
;
entry:
  ret i32 1

bb.1:
  ret i32 2

bb.2:
  br i1 undef, label %bb.1, label %bb.3

bb.3:
  ret i32 3
}

define i32 @remove_dead_block_with_phi() {
; CHECK-LABEL: @remove_dead_block_with_phi(
; CHECK-NEXT:  entry:
; CHECK-NEXT:    br label [[BB_2:%.*]]
; CHECK:       bb.2:
; CHECK-NEXT:    ret i32 1
; CHECK-NEXT:  }
;
entry:
  br label %bb.2

bb.1:
  br label %bb.2

bb.2:
  %rv = phi i32 [ 1, %entry ], [ 2, %bb.1 ]
  ret i32 %rv
}

define i32 @remove_dead_blocks_remaining_uses(i32 %a) {
; CHECK-LABEL: @remove_dead_blocks_remaining_uses(
; CHECK-NEXT:  entry:
; CHECK-NEXT:    ret i32 1
; CHECK-NEXT:  }
;
entry:
  ret i32 1

bb.2:
  ret i32 %res

bb.1:
  %res = add i32 %a, 10
  br label %bb.2
}

define i32 @remove_dead_blocks_remaining_uses2(i32 %a, i1 %cond) {
; CHECK-LABEL: @remove_dead_blocks_remaining_uses2(
; CHECK-NEXT:  entry:
; CHECK-NEXT:    ret i32 1
; CHECK:       bb.2:
; CHECK-NEXT:    [[RES2:%.*]] = add i32 undef, 10
; CHECK-NEXT:    [[RES3:%.*]] = mul i32 [[RES2]], undef
; CHECK-NEXT:    ret i32 [[RES3]]
; CHECK:       bb.3:
; CHECK-NEXT:    ret i32 undef
; CHECK-NEXT:  }
;
entry:
  ret i32 1

bb.2:
  %res2 = add i32 %res, 10
  %res3 = mul i32 %res2, %res
  ret i32 %res3

bb.3:
  br label %bb.4

bb.4:
  ret i32 %res

bb.1:
  %res = add i32 %a, 10
  br i1 %cond, label %bb.2, label %bb.3
  br label %bb.2
}
