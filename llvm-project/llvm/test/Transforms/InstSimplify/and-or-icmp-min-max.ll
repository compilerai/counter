; NOTE: Assertions have been autogenerated by utils/update_test_checks.py
; RUN: opt < %s -instsimplify -S | FileCheck %s

; There are 12 basic patterns (or 6 with DeMorganized equivalent) with
;    2 (commute logic op) *
;    2 (swap compare operands) *
;    2 (signed/unsigned)
; variations for a total of 96 tests.

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; (X == MAX) && (X < Y) --> false
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

define i1 @slt_and_max(i8 %x, i8 %y)  {
; CHECK-LABEL: @slt_and_max(
; CHECK-NEXT:    [[CMP:%.*]] = icmp slt i8 [[X:%.*]], [[Y:%.*]]
; CHECK-NEXT:    [[CMPEQ:%.*]] = icmp eq i8 [[X]], 127
; CHECK-NEXT:    [[R:%.*]] = and i1 [[CMP]], [[CMPEQ]]
; CHECK-NEXT:    ret i1 [[R]]
;
  %cmp = icmp slt i8 %x, %y
  %cmpeq = icmp eq i8 %x, 127
  %r = and i1 %cmp, %cmpeq
  ret i1 %r
}

define <2 x i1> @slt_and_max_commute(<2 x i8> %x, <2 x i8> %y)  {
; CHECK-LABEL: @slt_and_max_commute(
; CHECK-NEXT:    [[CMP:%.*]] = icmp slt <2 x i8> [[X:%.*]], [[Y:%.*]]
; CHECK-NEXT:    [[CMPEQ:%.*]] = icmp eq <2 x i8> [[X]], <i8 127, i8 127>
; CHECK-NEXT:    [[R:%.*]] = and <2 x i1> [[CMPEQ]], [[CMP]]
; CHECK-NEXT:    ret <2 x i1> [[R]]
;
  %cmp = icmp slt <2 x i8> %x, %y
  %cmpeq = icmp eq <2 x i8> %x, <i8 127, i8 127>
  %r = and <2 x i1> %cmpeq, %cmp
  ret <2 x i1> %r
}

define i1 @slt_swap_and_max(i8 %x, i8 %y)  {
; CHECK-LABEL: @slt_swap_and_max(
; CHECK-NEXT:    [[CMP:%.*]] = icmp sgt i8 [[Y:%.*]], [[X:%.*]]
; CHECK-NEXT:    [[CMPEQ:%.*]] = icmp eq i8 [[X]], 127
; CHECK-NEXT:    [[R:%.*]] = and i1 [[CMP]], [[CMPEQ]]
; CHECK-NEXT:    ret i1 [[R]]
;
  %cmp = icmp sgt i8 %y, %x
  %cmpeq = icmp eq i8 %x, 127
  %r = and i1 %cmp, %cmpeq
  ret i1 %r
}

define i1 @slt_swap_and_max_commute(i8 %x, i8 %y)  {
; CHECK-LABEL: @slt_swap_and_max_commute(
; CHECK-NEXT:    [[CMP:%.*]] = icmp sgt i8 [[Y:%.*]], [[X:%.*]]
; CHECK-NEXT:    [[CMPEQ:%.*]] = icmp eq i8 [[X]], 127
; CHECK-NEXT:    [[R:%.*]] = and i1 [[CMPEQ]], [[CMP]]
; CHECK-NEXT:    ret i1 [[R]]
;
  %cmp = icmp sgt i8 %y, %x
  %cmpeq = icmp eq i8 %x, 127
  %r = and i1 %cmpeq, %cmp
  ret i1 %r
}

define i1 @ult_and_max(i8 %x, i8 %y)  {
; CHECK-LABEL: @ult_and_max(
; CHECK-NEXT:    [[CMP:%.*]] = icmp ult i8 [[X:%.*]], [[Y:%.*]]
; CHECK-NEXT:    [[CMPEQ:%.*]] = icmp eq i8 [[X]], -1
; CHECK-NEXT:    [[R:%.*]] = and i1 [[CMP]], [[CMPEQ]]
; CHECK-NEXT:    ret i1 [[R]]
;
  %cmp = icmp ult i8 %x, %y
  %cmpeq = icmp eq i8 %x, 255
  %r = and i1 %cmp, %cmpeq
  ret i1 %r
}

define i1 @ult_and_max_commute(i8 %x, i8 %y)  {
; CHECK-LABEL: @ult_and_max_commute(
; CHECK-NEXT:    [[CMP:%.*]] = icmp ult i8 [[X:%.*]], [[Y:%.*]]
; CHECK-NEXT:    [[CMPEQ:%.*]] = icmp eq i8 [[X]], -1
; CHECK-NEXT:    [[R:%.*]] = and i1 [[CMPEQ]], [[CMP]]
; CHECK-NEXT:    ret i1 [[R]]
;
  %cmp = icmp ult i8 %x, %y
  %cmpeq = icmp eq i8 %x, 255
  %r = and i1 %cmpeq, %cmp
  ret i1 %r
}

define i1 @ult_swap_and_max(i8 %x, i8 %y)  {
; CHECK-LABEL: @ult_swap_and_max(
; CHECK-NEXT:    [[CMP:%.*]] = icmp ugt i8 [[Y:%.*]], [[X:%.*]]
; CHECK-NEXT:    [[CMPEQ:%.*]] = icmp eq i8 [[X]], -1
; CHECK-NEXT:    [[R:%.*]] = and i1 [[CMP]], [[CMPEQ]]
; CHECK-NEXT:    ret i1 [[R]]
;
  %cmp = icmp ugt i8 %y, %x
  %cmpeq = icmp eq i8 %x, 255
  %r = and i1 %cmp, %cmpeq
  ret i1 %r
}

define i1 @ult_swap_and_max_commute(i8 %x, i8 %y)  {
; CHECK-LABEL: @ult_swap_and_max_commute(
; CHECK-NEXT:    [[CMP:%.*]] = icmp ugt i8 [[Y:%.*]], [[X:%.*]]
; CHECK-NEXT:    [[CMPEQ:%.*]] = icmp eq i8 [[X]], -1
; CHECK-NEXT:    [[R:%.*]] = and i1 [[CMPEQ]], [[CMP]]
; CHECK-NEXT:    ret i1 [[R]]
;
  %cmp = icmp ugt i8 %y, %x
  %cmpeq = icmp eq i8 %x, 255
  %r = and i1 %cmpeq, %cmp
  ret i1 %r
}

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; (X == MIN) && (X > Y) --> false
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

define i1 @sgt_and_min(i9 %x, i9 %y)  {
; CHECK-LABEL: @sgt_and_min(
; CHECK-NEXT:    [[CMP:%.*]] = icmp sgt i9 [[X:%.*]], [[Y:%.*]]
; CHECK-NEXT:    [[CMPEQ:%.*]] = icmp eq i9 [[X]], -256
; CHECK-NEXT:    [[R:%.*]] = and i1 [[CMP]], [[CMPEQ]]
; CHECK-NEXT:    ret i1 [[R]]
;
  %cmp = icmp sgt i9 %x, %y
  %cmpeq = icmp eq i9 %x, 256
  %r = and i1 %cmp, %cmpeq
  ret i1 %r
}

define i1 @sgt_and_min_commute(i8 %x, i8 %y)  {
; CHECK-LABEL: @sgt_and_min_commute(
; CHECK-NEXT:    [[CMP:%.*]] = icmp sgt i8 [[X:%.*]], [[Y:%.*]]
; CHECK-NEXT:    [[CMPEQ:%.*]] = icmp eq i8 [[X]], -128
; CHECK-NEXT:    [[R:%.*]] = and i1 [[CMPEQ]], [[CMP]]
; CHECK-NEXT:    ret i1 [[R]]
;
  %cmp = icmp sgt i8 %x, %y
  %cmpeq = icmp eq i8 %x, 128
  %r = and i1 %cmpeq, %cmp
  ret i1 %r
}

define i1 @sgt_swap_and_min(i8 %x, i8 %y)  {
; CHECK-LABEL: @sgt_swap_and_min(
; CHECK-NEXT:    [[CMP:%.*]] = icmp slt i8 [[Y:%.*]], [[X:%.*]]
; CHECK-NEXT:    [[CMPEQ:%.*]] = icmp eq i8 [[X]], -128
; CHECK-NEXT:    [[R:%.*]] = and i1 [[CMP]], [[CMPEQ]]
; CHECK-NEXT:    ret i1 [[R]]
;
  %cmp = icmp slt i8 %y, %x
  %cmpeq = icmp eq i8 %x, 128
  %r = and i1 %cmp, %cmpeq
  ret i1 %r
}

define i1 @sgt_swap_and_min_commute(i8 %x, i8 %y)  {
; CHECK-LABEL: @sgt_swap_and_min_commute(
; CHECK-NEXT:    [[CMP:%.*]] = icmp slt i8 [[Y:%.*]], [[X:%.*]]
; CHECK-NEXT:    [[CMPEQ:%.*]] = icmp eq i8 [[X]], -128
; CHECK-NEXT:    [[R:%.*]] = and i1 [[CMPEQ]], [[CMP]]
; CHECK-NEXT:    ret i1 [[R]]
;
  %cmp = icmp slt i8 %y, %x
  %cmpeq = icmp eq i8 %x, 128
  %r = and i1 %cmpeq, %cmp
  ret i1 %r
}

define i1 @ugt_and_min(i8 %x, i8 %y)  {
; CHECK-LABEL: @ugt_and_min(
; CHECK-NEXT:    ret i1 false
;
  %cmp = icmp ugt i8 %x, %y
  %cmpeq = icmp eq i8 %x, 0
  %r = and i1 %cmp, %cmpeq
  ret i1 %r
}

define i1 @ugt_and_min_commute(i8 %x, i8 %y)  {
; CHECK-LABEL: @ugt_and_min_commute(
; CHECK-NEXT:    ret i1 false
;
  %cmp = icmp ugt i8 %x, %y
  %cmpeq = icmp eq i8 %x, 0
  %r = and i1 %cmpeq, %cmp
  ret i1 %r
}

define i1 @ugt_swap_and_min(i8 %x, i8 %y)  {
; CHECK-LABEL: @ugt_swap_and_min(
; CHECK-NEXT:    ret i1 false
;
  %cmp = icmp ult i8 %y, %x
  %cmpeq = icmp eq i8 %x, 0
  %r = and i1 %cmp, %cmpeq
  ret i1 %r
}

define i1 @ugt_swap_and_min_commute(i8 %x, i8 %y)  {
; CHECK-LABEL: @ugt_swap_and_min_commute(
; CHECK-NEXT:    ret i1 false
;
  %cmp = icmp ult i8 %y, %x
  %cmpeq = icmp eq i8 %x, 0
  %r = and i1 %cmpeq, %cmp
  ret i1 %r
}

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; (X != MAX) || (X >= Y) --> true
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

define i1 @sge_or_not_max(i8 %x, i8 %y)  {
; CHECK-LABEL: @sge_or_not_max(
; CHECK-NEXT:    [[CMP:%.*]] = icmp sge i8 [[X:%.*]], [[Y:%.*]]
; CHECK-NEXT:    [[CMPEQ:%.*]] = icmp ne i8 [[X]], 127
; CHECK-NEXT:    [[R:%.*]] = or i1 [[CMP]], [[CMPEQ]]
; CHECK-NEXT:    ret i1 [[R]]
;
  %cmp = icmp sge i8 %x, %y
  %cmpeq = icmp ne i8 %x, 127
  %r = or i1 %cmp, %cmpeq
  ret i1 %r
}

define i1 @sge_or_not_max_commute(i8 %x, i8 %y)  {
; CHECK-LABEL: @sge_or_not_max_commute(
; CHECK-NEXT:    [[CMP:%.*]] = icmp sge i8 [[X:%.*]], [[Y:%.*]]
; CHECK-NEXT:    [[CMPEQ:%.*]] = icmp ne i8 [[X]], 127
; CHECK-NEXT:    [[R:%.*]] = or i1 [[CMPEQ]], [[CMP]]
; CHECK-NEXT:    ret i1 [[R]]
;
  %cmp = icmp sge i8 %x, %y
  %cmpeq = icmp ne i8 %x, 127
  %r = or i1 %cmpeq, %cmp
  ret i1 %r
}

define i1 @sge_swap_or_not_max(i8 %x, i8 %y)  {
; CHECK-LABEL: @sge_swap_or_not_max(
; CHECK-NEXT:    [[CMP:%.*]] = icmp sle i8 [[Y:%.*]], [[X:%.*]]
; CHECK-NEXT:    [[CMPEQ:%.*]] = icmp ne i8 [[X]], 127
; CHECK-NEXT:    [[R:%.*]] = or i1 [[CMP]], [[CMPEQ]]
; CHECK-NEXT:    ret i1 [[R]]
;
  %cmp = icmp sle i8 %y, %x
  %cmpeq = icmp ne i8 %x, 127
  %r = or i1 %cmp, %cmpeq
  ret i1 %r
}

define i1 @sge_swap_or_not_max_commute(i8 %x, i8 %y)  {
; CHECK-LABEL: @sge_swap_or_not_max_commute(
; CHECK-NEXT:    [[CMP:%.*]] = icmp sle i8 [[Y:%.*]], [[X:%.*]]
; CHECK-NEXT:    [[CMPEQ:%.*]] = icmp ne i8 [[X]], 127
; CHECK-NEXT:    [[R:%.*]] = or i1 [[CMPEQ]], [[CMP]]
; CHECK-NEXT:    ret i1 [[R]]
;
  %cmp = icmp sle i8 %y, %x
  %cmpeq = icmp ne i8 %x, 127
  %r = or i1 %cmpeq, %cmp
  ret i1 %r
}

define i1 @uge_or_not_max(i8 %x, i8 %y)  {
; CHECK-LABEL: @uge_or_not_max(
; CHECK-NEXT:    [[CMP:%.*]] = icmp uge i8 [[X:%.*]], [[Y:%.*]]
; CHECK-NEXT:    [[CMPEQ:%.*]] = icmp ne i8 [[X]], -1
; CHECK-NEXT:    [[R:%.*]] = or i1 [[CMP]], [[CMPEQ]]
; CHECK-NEXT:    ret i1 [[R]]
;
  %cmp = icmp uge i8 %x, %y
  %cmpeq = icmp ne i8 %x, 255
  %r = or i1 %cmp, %cmpeq
  ret i1 %r
}

define i1 @uge_or_not_max_commute(i8 %x, i8 %y)  {
; CHECK-LABEL: @uge_or_not_max_commute(
; CHECK-NEXT:    [[CMP:%.*]] = icmp uge i8 [[X:%.*]], [[Y:%.*]]
; CHECK-NEXT:    [[CMPEQ:%.*]] = icmp ne i8 [[X]], -1
; CHECK-NEXT:    [[R:%.*]] = or i1 [[CMPEQ]], [[CMP]]
; CHECK-NEXT:    ret i1 [[R]]
;
  %cmp = icmp uge i8 %x, %y
  %cmpeq = icmp ne i8 %x, 255
  %r = or i1 %cmpeq, %cmp
  ret i1 %r
}

define i1 @uge_swap_or_not_max(i8 %x, i8 %y)  {
; CHECK-LABEL: @uge_swap_or_not_max(
; CHECK-NEXT:    [[CMP:%.*]] = icmp ule i8 [[Y:%.*]], [[X:%.*]]
; CHECK-NEXT:    [[CMPEQ:%.*]] = icmp ne i8 [[X]], -1
; CHECK-NEXT:    [[R:%.*]] = or i1 [[CMP]], [[CMPEQ]]
; CHECK-NEXT:    ret i1 [[R]]
;
  %cmp = icmp ule i8 %y, %x
  %cmpeq = icmp ne i8 %x, 255
  %r = or i1 %cmp, %cmpeq
  ret i1 %r
}

define i1 @uge_swap_or_not_max_commute(i8 %x, i8 %y)  {
; CHECK-LABEL: @uge_swap_or_not_max_commute(
; CHECK-NEXT:    [[CMP:%.*]] = icmp ule i8 [[Y:%.*]], [[X:%.*]]
; CHECK-NEXT:    [[CMPEQ:%.*]] = icmp ne i8 [[X]], -1
; CHECK-NEXT:    [[R:%.*]] = or i1 [[CMPEQ]], [[CMP]]
; CHECK-NEXT:    ret i1 [[R]]
;
  %cmp = icmp ule i8 %y, %x
  %cmpeq = icmp ne i8 %x, 255
  %r = or i1 %cmpeq, %cmp
  ret i1 %r
}

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; (X != MIN) || (X <= Y) --> true
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

define i1 @sle_or_not_min(i8 %x, i8 %y)  {
; CHECK-LABEL: @sle_or_not_min(
; CHECK-NEXT:    [[CMP:%.*]] = icmp sle i8 [[X:%.*]], [[Y:%.*]]
; CHECK-NEXT:    [[CMPEQ:%.*]] = icmp ne i8 [[X]], -128
; CHECK-NEXT:    [[R:%.*]] = or i1 [[CMP]], [[CMPEQ]]
; CHECK-NEXT:    ret i1 [[R]]
;
  %cmp = icmp sle i8 %x, %y
  %cmpeq = icmp ne i8 %x, 128
  %r = or i1 %cmp, %cmpeq
  ret i1 %r
}

define i1 @sle_or_not_min_commute(i8 %x, i8 %y)  {
; CHECK-LABEL: @sle_or_not_min_commute(
; CHECK-NEXT:    [[CMP:%.*]] = icmp sle i8 [[X:%.*]], [[Y:%.*]]
; CHECK-NEXT:    [[CMPEQ:%.*]] = icmp ne i8 [[X]], -128
; CHECK-NEXT:    [[R:%.*]] = or i1 [[CMPEQ]], [[CMP]]
; CHECK-NEXT:    ret i1 [[R]]
;
  %cmp = icmp sle i8 %x, %y
  %cmpeq = icmp ne i8 %x, 128
  %r = or i1 %cmpeq, %cmp
  ret i1 %r
}

define i1 @sle_swap_or_not_min(i8 %x, i8 %y)  {
; CHECK-LABEL: @sle_swap_or_not_min(
; CHECK-NEXT:    [[CMP:%.*]] = icmp sge i8 [[Y:%.*]], [[X:%.*]]
; CHECK-NEXT:    [[CMPEQ:%.*]] = icmp ne i8 [[X]], -128
; CHECK-NEXT:    [[R:%.*]] = or i1 [[CMP]], [[CMPEQ]]
; CHECK-NEXT:    ret i1 [[R]]
;
  %cmp = icmp sge i8 %y, %x
  %cmpeq = icmp ne i8 %x, 128
  %r = or i1 %cmp, %cmpeq
  ret i1 %r
}

define i1 @sle_swap_or_not_min_commute(i8 %x, i8 %y)  {
; CHECK-LABEL: @sle_swap_or_not_min_commute(
; CHECK-NEXT:    [[CMP:%.*]] = icmp sge i8 [[Y:%.*]], [[X:%.*]]
; CHECK-NEXT:    [[CMPEQ:%.*]] = icmp ne i8 [[X]], -128
; CHECK-NEXT:    [[R:%.*]] = or i1 [[CMPEQ]], [[CMP]]
; CHECK-NEXT:    ret i1 [[R]]
;
  %cmp = icmp sge i8 %y, %x
  %cmpeq = icmp ne i8 %x, 128
  %r = or i1 %cmpeq, %cmp
  ret i1 %r
}

define i1 @ule_or_not_min(i427 %x, i427 %y)  {
; CHECK-LABEL: @ule_or_not_min(
; CHECK-NEXT:    ret i1 true
;
  %cmp = icmp ule i427 %x, %y
  %cmpeq = icmp ne i427 %x, 0
  %r = or i1 %cmp, %cmpeq
  ret i1 %r
}

define i1 @ule_or_not_min_commute(i8 %x, i8 %y)  {
; CHECK-LABEL: @ule_or_not_min_commute(
; CHECK-NEXT:    ret i1 true
;
  %cmp = icmp ule i8 %x, %y
  %cmpeq = icmp ne i8 %x, 0
  %r = or i1 %cmpeq, %cmp
  ret i1 %r
}

define i1 @ule_swap_or_not_min(i8 %x, i8 %y)  {
; CHECK-LABEL: @ule_swap_or_not_min(
; CHECK-NEXT:    ret i1 true
;
  %cmp = icmp uge i8 %y, %x
  %cmpeq = icmp ne i8 %x, 0
  %r = or i1 %cmp, %cmpeq
  ret i1 %r
}

define i1 @ule_swap_or_not_min_commute(i8 %x, i8 %y)  {
; CHECK-LABEL: @ule_swap_or_not_min_commute(
; CHECK-NEXT:    ret i1 true
;
  %cmp = icmp uge i8 %y, %x
  %cmpeq = icmp ne i8 %x, 0
  %r = or i1 %cmpeq, %cmp
  ret i1 %r
}

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; (X == MAX) && (X >= Y) --> X == MAX
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

define i1 @sge_and_max(i8 %x, i8 %y)  {
; CHECK-LABEL: @sge_and_max(
; CHECK-NEXT:    [[CMP:%.*]] = icmp sge i8 [[X:%.*]], [[Y:%.*]]
; CHECK-NEXT:    [[CMPEQ:%.*]] = icmp eq i8 [[X]], 127
; CHECK-NEXT:    [[R:%.*]] = and i1 [[CMP]], [[CMPEQ]]
; CHECK-NEXT:    ret i1 [[R]]
;
  %cmp = icmp sge i8 %x, %y
  %cmpeq = icmp eq i8 %x, 127
  %r = and i1 %cmp, %cmpeq
  ret i1 %r
}

define i1 @sge_and_max_commute(i8 %x, i8 %y)  {
; CHECK-LABEL: @sge_and_max_commute(
; CHECK-NEXT:    [[CMP:%.*]] = icmp sge i8 [[X:%.*]], [[Y:%.*]]
; CHECK-NEXT:    [[CMPEQ:%.*]] = icmp eq i8 [[X]], 127
; CHECK-NEXT:    [[R:%.*]] = and i1 [[CMPEQ]], [[CMP]]
; CHECK-NEXT:    ret i1 [[R]]
;
  %cmp = icmp sge i8 %x, %y
  %cmpeq = icmp eq i8 %x, 127
  %r = and i1 %cmpeq, %cmp
  ret i1 %r
}

define i1 @sge_swap_and_max(i8 %x, i8 %y)  {
; CHECK-LABEL: @sge_swap_and_max(
; CHECK-NEXT:    [[CMP:%.*]] = icmp sle i8 [[Y:%.*]], [[X:%.*]]
; CHECK-NEXT:    [[CMPEQ:%.*]] = icmp eq i8 [[X]], 127
; CHECK-NEXT:    [[R:%.*]] = and i1 [[CMP]], [[CMPEQ]]
; CHECK-NEXT:    ret i1 [[R]]
;
  %cmp = icmp sle i8 %y, %x
  %cmpeq = icmp eq i8 %x, 127
  %r = and i1 %cmp, %cmpeq
  ret i1 %r
}

define i1 @sge_swap_and_max_commute(i8 %x, i8 %y)  {
; CHECK-LABEL: @sge_swap_and_max_commute(
; CHECK-NEXT:    [[CMP:%.*]] = icmp sle i8 [[Y:%.*]], [[X:%.*]]
; CHECK-NEXT:    [[CMPEQ:%.*]] = icmp eq i8 [[X]], 127
; CHECK-NEXT:    [[R:%.*]] = and i1 [[CMPEQ]], [[CMP]]
; CHECK-NEXT:    ret i1 [[R]]
;
  %cmp = icmp sle i8 %y, %x
  %cmpeq = icmp eq i8 %x, 127
  %r = and i1 %cmpeq, %cmp
  ret i1 %r
}

define i1 @uge_and_max(i8 %x, i8 %y)  {
; CHECK-LABEL: @uge_and_max(
; CHECK-NEXT:    [[CMP:%.*]] = icmp uge i8 [[X:%.*]], [[Y:%.*]]
; CHECK-NEXT:    [[CMPEQ:%.*]] = icmp eq i8 [[X]], -1
; CHECK-NEXT:    [[R:%.*]] = and i1 [[CMP]], [[CMPEQ]]
; CHECK-NEXT:    ret i1 [[R]]
;
  %cmp = icmp uge i8 %x, %y
  %cmpeq = icmp eq i8 %x, 255
  %r = and i1 %cmp, %cmpeq
  ret i1 %r
}

define i1 @uge_and_max_commute(i8 %x, i8 %y)  {
; CHECK-LABEL: @uge_and_max_commute(
; CHECK-NEXT:    [[CMP:%.*]] = icmp uge i8 [[X:%.*]], [[Y:%.*]]
; CHECK-NEXT:    [[CMPEQ:%.*]] = icmp eq i8 [[X]], -1
; CHECK-NEXT:    [[R:%.*]] = and i1 [[CMPEQ]], [[CMP]]
; CHECK-NEXT:    ret i1 [[R]]
;
  %cmp = icmp uge i8 %x, %y
  %cmpeq = icmp eq i8 %x, 255
  %r = and i1 %cmpeq, %cmp
  ret i1 %r
}

define i1 @uge_swap_and_max(i8 %x, i8 %y)  {
; CHECK-LABEL: @uge_swap_and_max(
; CHECK-NEXT:    [[CMP:%.*]] = icmp ule i8 [[Y:%.*]], [[X:%.*]]
; CHECK-NEXT:    [[CMPEQ:%.*]] = icmp eq i8 [[X]], -1
; CHECK-NEXT:    [[R:%.*]] = and i1 [[CMP]], [[CMPEQ]]
; CHECK-NEXT:    ret i1 [[R]]
;
  %cmp = icmp ule i8 %y, %x
  %cmpeq = icmp eq i8 %x, 255
  %r = and i1 %cmp, %cmpeq
  ret i1 %r
}

define i1 @uge_swap_and_max_commute(i8 %x, i8 %y)  {
; CHECK-LABEL: @uge_swap_and_max_commute(
; CHECK-NEXT:    [[CMP:%.*]] = icmp ule i8 [[Y:%.*]], [[X:%.*]]
; CHECK-NEXT:    [[CMPEQ:%.*]] = icmp eq i8 [[X]], -1
; CHECK-NEXT:    [[R:%.*]] = and i1 [[CMPEQ]], [[CMP]]
; CHECK-NEXT:    ret i1 [[R]]
;
  %cmp = icmp ule i8 %y, %x
  %cmpeq = icmp eq i8 %x, 255
  %r = and i1 %cmpeq, %cmp
  ret i1 %r
}

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; (X == MIN) && (X <= Y) --> X == MIN
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

define i1 @sle_and_min(i8 %x, i8 %y)  {
; CHECK-LABEL: @sle_and_min(
; CHECK-NEXT:    [[CMP:%.*]] = icmp sle i8 [[X:%.*]], [[Y:%.*]]
; CHECK-NEXT:    [[CMPEQ:%.*]] = icmp eq i8 [[X]], -128
; CHECK-NEXT:    [[R:%.*]] = and i1 [[CMP]], [[CMPEQ]]
; CHECK-NEXT:    ret i1 [[R]]
;
  %cmp = icmp sle i8 %x, %y
  %cmpeq = icmp eq i8 %x, 128
  %r = and i1 %cmp, %cmpeq
  ret i1 %r
}

define i1 @sle_and_min_commute(i8 %x, i8 %y)  {
; CHECK-LABEL: @sle_and_min_commute(
; CHECK-NEXT:    [[CMP:%.*]] = icmp sle i8 [[X:%.*]], [[Y:%.*]]
; CHECK-NEXT:    [[CMPEQ:%.*]] = icmp eq i8 [[X]], -128
; CHECK-NEXT:    [[R:%.*]] = and i1 [[CMPEQ]], [[CMP]]
; CHECK-NEXT:    ret i1 [[R]]
;
  %cmp = icmp sle i8 %x, %y
  %cmpeq = icmp eq i8 %x, 128
  %r = and i1 %cmpeq, %cmp
  ret i1 %r
}

define i1 @sle_swap_and_min(i8 %x, i8 %y)  {
; CHECK-LABEL: @sle_swap_and_min(
; CHECK-NEXT:    [[CMP:%.*]] = icmp sge i8 [[Y:%.*]], [[X:%.*]]
; CHECK-NEXT:    [[CMPEQ:%.*]] = icmp eq i8 [[X]], -128
; CHECK-NEXT:    [[R:%.*]] = and i1 [[CMP]], [[CMPEQ]]
; CHECK-NEXT:    ret i1 [[R]]
;
  %cmp = icmp sge i8 %y, %x
  %cmpeq = icmp eq i8 %x, 128
  %r = and i1 %cmp, %cmpeq
  ret i1 %r
}

define i1 @sle_swap_and_min_commute(i8 %x, i8 %y)  {
; CHECK-LABEL: @sle_swap_and_min_commute(
; CHECK-NEXT:    [[CMP:%.*]] = icmp sge i8 [[Y:%.*]], [[X:%.*]]
; CHECK-NEXT:    [[CMPEQ:%.*]] = icmp eq i8 [[X]], -128
; CHECK-NEXT:    [[R:%.*]] = and i1 [[CMPEQ]], [[CMP]]
; CHECK-NEXT:    ret i1 [[R]]
;
  %cmp = icmp sge i8 %y, %x
  %cmpeq = icmp eq i8 %x, 128
  %r = and i1 %cmpeq, %cmp
  ret i1 %r
}

define i1 @ule_and_min(i8 %x, i8 %y)  {
; CHECK-LABEL: @ule_and_min(
; CHECK-NEXT:    [[CMPEQ:%.*]] = icmp eq i8 [[X:%.*]], 0
; CHECK-NEXT:    ret i1 [[CMPEQ]]
;
  %cmp = icmp ule i8 %x, %y
  %cmpeq = icmp eq i8 %x, 0
  %r = and i1 %cmp, %cmpeq
  ret i1 %r
}

define i1 @ule_and_min_commute(i8 %x, i8 %y)  {
; CHECK-LABEL: @ule_and_min_commute(
; CHECK-NEXT:    [[CMPEQ:%.*]] = icmp eq i8 [[X:%.*]], 0
; CHECK-NEXT:    ret i1 [[CMPEQ]]
;
  %cmp = icmp ule i8 %x, %y
  %cmpeq = icmp eq i8 %x, 0
  %r = and i1 %cmpeq, %cmp
  ret i1 %r
}

define i1 @ule_swap_and_min(i8 %x, i8 %y)  {
; CHECK-LABEL: @ule_swap_and_min(
; CHECK-NEXT:    [[CMPEQ:%.*]] = icmp eq i8 [[X:%.*]], 0
; CHECK-NEXT:    ret i1 [[CMPEQ]]
;
  %cmp = icmp uge i8 %y, %x
  %cmpeq = icmp eq i8 %x, 0
  %r = and i1 %cmp, %cmpeq
  ret i1 %r
}

define i1 @ule_swap_and_min_commute(i8 %x, i8 %y)  {
; CHECK-LABEL: @ule_swap_and_min_commute(
; CHECK-NEXT:    [[CMPEQ:%.*]] = icmp eq i8 [[X:%.*]], 0
; CHECK-NEXT:    ret i1 [[CMPEQ]]
;
  %cmp = icmp uge i8 %y, %x
  %cmpeq = icmp eq i8 %x, 0
  %r = and i1 %cmpeq, %cmp
  ret i1 %r
}

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; (X == MAX) || (X >= Y) --> X >= Y
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

define i1 @sge_or_max(i8 %x, i8 %y)  {
; CHECK-LABEL: @sge_or_max(
; CHECK-NEXT:    [[CMP:%.*]] = icmp sge i8 [[X:%.*]], [[Y:%.*]]
; CHECK-NEXT:    [[CMPEQ:%.*]] = icmp eq i8 [[X]], 127
; CHECK-NEXT:    [[R:%.*]] = or i1 [[CMP]], [[CMPEQ]]
; CHECK-NEXT:    ret i1 [[R]]
;
  %cmp = icmp sge i8 %x, %y
  %cmpeq = icmp eq i8 %x, 127
  %r = or i1 %cmp, %cmpeq
  ret i1 %r
}

define i1 @sge_or_max_commute(i8 %x, i8 %y)  {
; CHECK-LABEL: @sge_or_max_commute(
; CHECK-NEXT:    [[CMP:%.*]] = icmp sge i8 [[X:%.*]], [[Y:%.*]]
; CHECK-NEXT:    [[CMPEQ:%.*]] = icmp eq i8 [[X]], 127
; CHECK-NEXT:    [[R:%.*]] = or i1 [[CMPEQ]], [[CMP]]
; CHECK-NEXT:    ret i1 [[R]]
;
  %cmp = icmp sge i8 %x, %y
  %cmpeq = icmp eq i8 %x, 127
  %r = or i1 %cmpeq, %cmp
  ret i1 %r
}

define i1 @sge_swap_or_max(i8 %x, i8 %y)  {
; CHECK-LABEL: @sge_swap_or_max(
; CHECK-NEXT:    [[CMP:%.*]] = icmp sle i8 [[Y:%.*]], [[X:%.*]]
; CHECK-NEXT:    [[CMPEQ:%.*]] = icmp eq i8 [[X]], 127
; CHECK-NEXT:    [[R:%.*]] = or i1 [[CMP]], [[CMPEQ]]
; CHECK-NEXT:    ret i1 [[R]]
;
  %cmp = icmp sle i8 %y, %x
  %cmpeq = icmp eq i8 %x, 127
  %r = or i1 %cmp, %cmpeq
  ret i1 %r
}

define i1 @sge_swap_or_max_commute(i8 %x, i8 %y)  {
; CHECK-LABEL: @sge_swap_or_max_commute(
; CHECK-NEXT:    [[CMP:%.*]] = icmp sle i8 [[Y:%.*]], [[X:%.*]]
; CHECK-NEXT:    [[CMPEQ:%.*]] = icmp eq i8 [[X]], 127
; CHECK-NEXT:    [[R:%.*]] = or i1 [[CMPEQ]], [[CMP]]
; CHECK-NEXT:    ret i1 [[R]]
;
  %cmp = icmp sle i8 %y, %x
  %cmpeq = icmp eq i8 %x, 127
  %r = or i1 %cmpeq, %cmp
  ret i1 %r
}

define i1 @uge_or_max(i8 %x, i8 %y)  {
; CHECK-LABEL: @uge_or_max(
; CHECK-NEXT:    [[CMP:%.*]] = icmp uge i8 [[X:%.*]], [[Y:%.*]]
; CHECK-NEXT:    [[CMPEQ:%.*]] = icmp eq i8 [[X]], -1
; CHECK-NEXT:    [[R:%.*]] = or i1 [[CMP]], [[CMPEQ]]
; CHECK-NEXT:    ret i1 [[R]]
;
  %cmp = icmp uge i8 %x, %y
  %cmpeq = icmp eq i8 %x, 255
  %r = or i1 %cmp, %cmpeq
  ret i1 %r
}

define i1 @uge_or_max_commute(i8 %x, i8 %y)  {
; CHECK-LABEL: @uge_or_max_commute(
; CHECK-NEXT:    [[CMP:%.*]] = icmp uge i8 [[X:%.*]], [[Y:%.*]]
; CHECK-NEXT:    [[CMPEQ:%.*]] = icmp eq i8 [[X]], -1
; CHECK-NEXT:    [[R:%.*]] = or i1 [[CMPEQ]], [[CMP]]
; CHECK-NEXT:    ret i1 [[R]]
;
  %cmp = icmp uge i8 %x, %y
  %cmpeq = icmp eq i8 %x, 255
  %r = or i1 %cmpeq, %cmp
  ret i1 %r
}

define i1 @uge_swap_or_max(i8 %x, i8 %y)  {
; CHECK-LABEL: @uge_swap_or_max(
; CHECK-NEXT:    [[CMP:%.*]] = icmp ule i8 [[Y:%.*]], [[X:%.*]]
; CHECK-NEXT:    [[CMPEQ:%.*]] = icmp eq i8 [[X]], -1
; CHECK-NEXT:    [[R:%.*]] = or i1 [[CMP]], [[CMPEQ]]
; CHECK-NEXT:    ret i1 [[R]]
;
  %cmp = icmp ule i8 %y, %x
  %cmpeq = icmp eq i8 %x, 255
  %r = or i1 %cmp, %cmpeq
  ret i1 %r
}

define i1 @uge_swap_or_max_commute(i8 %x, i8 %y)  {
; CHECK-LABEL: @uge_swap_or_max_commute(
; CHECK-NEXT:    [[CMP:%.*]] = icmp ule i8 [[Y:%.*]], [[X:%.*]]
; CHECK-NEXT:    [[CMPEQ:%.*]] = icmp eq i8 [[X]], -1
; CHECK-NEXT:    [[R:%.*]] = or i1 [[CMPEQ]], [[CMP]]
; CHECK-NEXT:    ret i1 [[R]]
;
  %cmp = icmp ule i8 %y, %x
  %cmpeq = icmp eq i8 %x, 255
  %r = or i1 %cmpeq, %cmp
  ret i1 %r
}

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; (X == MIN) || (X <= Y) --> X <= Y
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

define i1 @sle_or_min(i8 %x, i8 %y)  {
; CHECK-LABEL: @sle_or_min(
; CHECK-NEXT:    [[CMP:%.*]] = icmp sle i8 [[X:%.*]], [[Y:%.*]]
; CHECK-NEXT:    [[CMPEQ:%.*]] = icmp eq i8 [[X]], -128
; CHECK-NEXT:    [[R:%.*]] = or i1 [[CMP]], [[CMPEQ]]
; CHECK-NEXT:    ret i1 [[R]]
;
  %cmp = icmp sle i8 %x, %y
  %cmpeq = icmp eq i8 %x, 128
  %r = or i1 %cmp, %cmpeq
  ret i1 %r
}

define i1 @sle_or_min_commute(i8 %x, i8 %y)  {
; CHECK-LABEL: @sle_or_min_commute(
; CHECK-NEXT:    [[CMP:%.*]] = icmp sle i8 [[X:%.*]], [[Y:%.*]]
; CHECK-NEXT:    [[CMPEQ:%.*]] = icmp eq i8 [[X]], -128
; CHECK-NEXT:    [[R:%.*]] = or i1 [[CMPEQ]], [[CMP]]
; CHECK-NEXT:    ret i1 [[R]]
;
  %cmp = icmp sle i8 %x, %y
  %cmpeq = icmp eq i8 %x, 128
  %r = or i1 %cmpeq, %cmp
  ret i1 %r
}

define i1 @sle_swap_or_min(i8 %x, i8 %y)  {
; CHECK-LABEL: @sle_swap_or_min(
; CHECK-NEXT:    [[CMP:%.*]] = icmp sge i8 [[Y:%.*]], [[X:%.*]]
; CHECK-NEXT:    [[CMPEQ:%.*]] = icmp eq i8 [[X]], -128
; CHECK-NEXT:    [[R:%.*]] = or i1 [[CMP]], [[CMPEQ]]
; CHECK-NEXT:    ret i1 [[R]]
;
  %cmp = icmp sge i8 %y, %x
  %cmpeq = icmp eq i8 %x, 128
  %r = or i1 %cmp, %cmpeq
  ret i1 %r
}

define i1 @sle_swap_or_min_commute(i8 %x, i8 %y)  {
; CHECK-LABEL: @sle_swap_or_min_commute(
; CHECK-NEXT:    [[CMP:%.*]] = icmp sge i8 [[Y:%.*]], [[X:%.*]]
; CHECK-NEXT:    [[CMPEQ:%.*]] = icmp eq i8 [[X]], -128
; CHECK-NEXT:    [[R:%.*]] = or i1 [[CMPEQ]], [[CMP]]
; CHECK-NEXT:    ret i1 [[R]]
;
  %cmp = icmp sge i8 %y, %x
  %cmpeq = icmp eq i8 %x, 128
  %r = or i1 %cmpeq, %cmp
  ret i1 %r
}

define i1 @ule_or_min(i8 %x, i8 %y)  {
; CHECK-LABEL: @ule_or_min(
; CHECK-NEXT:    [[CMP:%.*]] = icmp ule i8 [[X:%.*]], [[Y:%.*]]
; CHECK-NEXT:    ret i1 [[CMP]]
;
  %cmp = icmp ule i8 %x, %y
  %cmpeq = icmp eq i8 %x, 0
  %r = or i1 %cmp, %cmpeq
  ret i1 %r
}

define i1 @ule_or_min_commute(i8 %x, i8 %y)  {
; CHECK-LABEL: @ule_or_min_commute(
; CHECK-NEXT:    [[CMP:%.*]] = icmp ule i8 [[X:%.*]], [[Y:%.*]]
; CHECK-NEXT:    ret i1 [[CMP]]
;
  %cmp = icmp ule i8 %x, %y
  %cmpeq = icmp eq i8 %x, 0
  %r = or i1 %cmpeq, %cmp
  ret i1 %r
}

define i1 @ule_swap_or_min(i8 %x, i8 %y)  {
; CHECK-LABEL: @ule_swap_or_min(
; CHECK-NEXT:    [[CMP:%.*]] = icmp uge i8 [[Y:%.*]], [[X:%.*]]
; CHECK-NEXT:    ret i1 [[CMP]]
;
  %cmp = icmp uge i8 %y, %x
  %cmpeq = icmp eq i8 %x, 0
  %r = or i1 %cmp, %cmpeq
  ret i1 %r
}

define i1 @ule_swap_or_min_commute(i8 %x, i8 %y)  {
; CHECK-LABEL: @ule_swap_or_min_commute(
; CHECK-NEXT:    [[CMP:%.*]] = icmp uge i8 [[Y:%.*]], [[X:%.*]]
; CHECK-NEXT:    ret i1 [[CMP]]
;
  %cmp = icmp uge i8 %y, %x
  %cmpeq = icmp eq i8 %x, 0
  %r = or i1 %cmpeq, %cmp
  ret i1 %r
}

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; (X != MAX) && (X < Y) --> X < Y
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

define i1 @slt_and_not_max(i8 %x, i8 %y)  {
; CHECK-LABEL: @slt_and_not_max(
; CHECK-NEXT:    [[CMP:%.*]] = icmp slt i8 [[X:%.*]], [[Y:%.*]]
; CHECK-NEXT:    [[CMPEQ:%.*]] = icmp ne i8 [[X]], 127
; CHECK-NEXT:    [[R:%.*]] = and i1 [[CMP]], [[CMPEQ]]
; CHECK-NEXT:    ret i1 [[R]]
;
  %cmp = icmp slt i8 %x, %y
  %cmpeq = icmp ne i8 %x, 127
  %r = and i1 %cmp, %cmpeq
  ret i1 %r
}

define i1 @slt_and_not_max_commute(i8 %x, i8 %y)  {
; CHECK-LABEL: @slt_and_not_max_commute(
; CHECK-NEXT:    [[CMP:%.*]] = icmp slt i8 [[X:%.*]], [[Y:%.*]]
; CHECK-NEXT:    [[CMPEQ:%.*]] = icmp ne i8 [[X]], 127
; CHECK-NEXT:    [[R:%.*]] = and i1 [[CMPEQ]], [[CMP]]
; CHECK-NEXT:    ret i1 [[R]]
;
  %cmp = icmp slt i8 %x, %y
  %cmpeq = icmp ne i8 %x, 127
  %r = and i1 %cmpeq, %cmp
  ret i1 %r
}

define i1 @slt_swap_and_not_max(i8 %x, i8 %y)  {
; CHECK-LABEL: @slt_swap_and_not_max(
; CHECK-NEXT:    [[CMP:%.*]] = icmp sgt i8 [[Y:%.*]], [[X:%.*]]
; CHECK-NEXT:    [[CMPEQ:%.*]] = icmp ne i8 [[X]], 127
; CHECK-NEXT:    [[R:%.*]] = and i1 [[CMP]], [[CMPEQ]]
; CHECK-NEXT:    ret i1 [[R]]
;
  %cmp = icmp sgt i8 %y, %x
  %cmpeq = icmp ne i8 %x, 127
  %r = and i1 %cmp, %cmpeq
  ret i1 %r
}

define i1 @slt_swap_and_not_max_commute(i8 %x, i8 %y)  {
; CHECK-LABEL: @slt_swap_and_not_max_commute(
; CHECK-NEXT:    [[CMP:%.*]] = icmp sgt i8 [[Y:%.*]], [[X:%.*]]
; CHECK-NEXT:    [[CMPEQ:%.*]] = icmp ne i8 [[X]], 127
; CHECK-NEXT:    [[R:%.*]] = and i1 [[CMPEQ]], [[CMP]]
; CHECK-NEXT:    ret i1 [[R]]
;
  %cmp = icmp sgt i8 %y, %x
  %cmpeq = icmp ne i8 %x, 127
  %r = and i1 %cmpeq, %cmp
  ret i1 %r
}

define i1 @ult_and_not_max(i8 %x, i8 %y)  {
; CHECK-LABEL: @ult_and_not_max(
; CHECK-NEXT:    [[CMP:%.*]] = icmp ult i8 [[X:%.*]], [[Y:%.*]]
; CHECK-NEXT:    [[CMPEQ:%.*]] = icmp ne i8 [[X]], -1
; CHECK-NEXT:    [[R:%.*]] = and i1 [[CMP]], [[CMPEQ]]
; CHECK-NEXT:    ret i1 [[R]]
;
  %cmp = icmp ult i8 %x, %y
  %cmpeq = icmp ne i8 %x, 255
  %r = and i1 %cmp, %cmpeq
  ret i1 %r
}

define i1 @ult_and_not_max_commute(i8 %x, i8 %y)  {
; CHECK-LABEL: @ult_and_not_max_commute(
; CHECK-NEXT:    [[CMP:%.*]] = icmp ult i8 [[X:%.*]], [[Y:%.*]]
; CHECK-NEXT:    [[CMPEQ:%.*]] = icmp ne i8 [[X]], -1
; CHECK-NEXT:    [[R:%.*]] = and i1 [[CMPEQ]], [[CMP]]
; CHECK-NEXT:    ret i1 [[R]]
;
  %cmp = icmp ult i8 %x, %y
  %cmpeq = icmp ne i8 %x, 255
  %r = and i1 %cmpeq, %cmp
  ret i1 %r
}

define i1 @ult_swap_and_not_max(i8 %x, i8 %y)  {
; CHECK-LABEL: @ult_swap_and_not_max(
; CHECK-NEXT:    [[CMP:%.*]] = icmp ugt i8 [[Y:%.*]], [[X:%.*]]
; CHECK-NEXT:    [[CMPEQ:%.*]] = icmp ne i8 [[X]], -1
; CHECK-NEXT:    [[R:%.*]] = and i1 [[CMP]], [[CMPEQ]]
; CHECK-NEXT:    ret i1 [[R]]
;
  %cmp = icmp ugt i8 %y, %x
  %cmpeq = icmp ne i8 %x, 255
  %r = and i1 %cmp, %cmpeq
  ret i1 %r
}

define i1 @ult_swap_and_not_max_commute(i8 %x, i8 %y)  {
; CHECK-LABEL: @ult_swap_and_not_max_commute(
; CHECK-NEXT:    [[CMP:%.*]] = icmp ugt i8 [[Y:%.*]], [[X:%.*]]
; CHECK-NEXT:    [[CMPEQ:%.*]] = icmp ne i8 [[X]], -1
; CHECK-NEXT:    [[R:%.*]] = and i1 [[CMPEQ]], [[CMP]]
; CHECK-NEXT:    ret i1 [[R]]
;
  %cmp = icmp ugt i8 %y, %x
  %cmpeq = icmp ne i8 %x, 255
  %r = and i1 %cmpeq, %cmp
  ret i1 %r
}

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; (X != MIN) && (X > Y) --> X > Y
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

define i1 @sgt_and_not_min(i8 %x, i8 %y)  {
; CHECK-LABEL: @sgt_and_not_min(
; CHECK-NEXT:    [[CMP:%.*]] = icmp sgt i8 [[X:%.*]], [[Y:%.*]]
; CHECK-NEXT:    [[CMPEQ:%.*]] = icmp ne i8 [[X]], -128
; CHECK-NEXT:    [[R:%.*]] = and i1 [[CMP]], [[CMPEQ]]
; CHECK-NEXT:    ret i1 [[R]]
;
  %cmp = icmp sgt i8 %x, %y
  %cmpeq = icmp ne i8 %x, 128
  %r = and i1 %cmp, %cmpeq
  ret i1 %r
}

define i1 @sgt_and_not_min_commute(i8 %x, i8 %y)  {
; CHECK-LABEL: @sgt_and_not_min_commute(
; CHECK-NEXT:    [[CMP:%.*]] = icmp sgt i8 [[X:%.*]], [[Y:%.*]]
; CHECK-NEXT:    [[CMPEQ:%.*]] = icmp ne i8 [[X]], -128
; CHECK-NEXT:    [[R:%.*]] = and i1 [[CMPEQ]], [[CMP]]
; CHECK-NEXT:    ret i1 [[R]]
;
  %cmp = icmp sgt i8 %x, %y
  %cmpeq = icmp ne i8 %x, 128
  %r = and i1 %cmpeq, %cmp
  ret i1 %r
}

define i1 @sgt_swap_and_not_min(i8 %x, i8 %y)  {
; CHECK-LABEL: @sgt_swap_and_not_min(
; CHECK-NEXT:    [[CMP:%.*]] = icmp slt i8 [[Y:%.*]], [[X:%.*]]
; CHECK-NEXT:    [[CMPEQ:%.*]] = icmp ne i8 [[X]], -128
; CHECK-NEXT:    [[R:%.*]] = and i1 [[CMP]], [[CMPEQ]]
; CHECK-NEXT:    ret i1 [[R]]
;
  %cmp = icmp slt i8 %y, %x
  %cmpeq = icmp ne i8 %x, 128
  %r = and i1 %cmp, %cmpeq
  ret i1 %r
}

define i1 @sgt_swap_and_not_min_commute(i8 %x, i8 %y)  {
; CHECK-LABEL: @sgt_swap_and_not_min_commute(
; CHECK-NEXT:    [[CMP:%.*]] = icmp slt i8 [[Y:%.*]], [[X:%.*]]
; CHECK-NEXT:    [[CMPEQ:%.*]] = icmp ne i8 [[X]], -128
; CHECK-NEXT:    [[R:%.*]] = and i1 [[CMPEQ]], [[CMP]]
; CHECK-NEXT:    ret i1 [[R]]
;
  %cmp = icmp slt i8 %y, %x
  %cmpeq = icmp ne i8 %x, 128
  %r = and i1 %cmpeq, %cmp
  ret i1 %r
}

define i1 @ugt_and_not_min(i8 %x, i8 %y)  {
; CHECK-LABEL: @ugt_and_not_min(
; CHECK-NEXT:    [[CMP:%.*]] = icmp ugt i8 [[X:%.*]], [[Y:%.*]]
; CHECK-NEXT:    ret i1 [[CMP]]
;
  %cmp = icmp ugt i8 %x, %y
  %cmpeq = icmp ne i8 %x, 0
  %r = and i1 %cmp, %cmpeq
  ret i1 %r
}

define i1 @ugt_and_not_min_commute(i8 %x, i8 %y)  {
; CHECK-LABEL: @ugt_and_not_min_commute(
; CHECK-NEXT:    [[CMP:%.*]] = icmp ugt i8 [[X:%.*]], [[Y:%.*]]
; CHECK-NEXT:    ret i1 [[CMP]]
;
  %cmp = icmp ugt i8 %x, %y
  %cmpeq = icmp ne i8 %x, 0
  %r = and i1 %cmpeq, %cmp
  ret i1 %r
}

define i1 @ugt_swap_and_not_min(i8 %x, i8 %y)  {
; CHECK-LABEL: @ugt_swap_and_not_min(
; CHECK-NEXT:    [[CMP:%.*]] = icmp ult i8 [[Y:%.*]], [[X:%.*]]
; CHECK-NEXT:    ret i1 [[CMP]]
;
  %cmp = icmp ult i8 %y, %x
  %cmpeq = icmp ne i8 %x, 0
  %r = and i1 %cmp, %cmpeq
  ret i1 %r
}

define i1 @ugt_swap_and_not_min_commute(i8 %x, i8 %y)  {
; CHECK-LABEL: @ugt_swap_and_not_min_commute(
; CHECK-NEXT:    [[CMP:%.*]] = icmp ult i8 [[Y:%.*]], [[X:%.*]]
; CHECK-NEXT:    ret i1 [[CMP]]
;
  %cmp = icmp ult i8 %y, %x
  %cmpeq = icmp ne i8 %x, 0
  %r = and i1 %cmpeq, %cmp
  ret i1 %r
}

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; (X != MAX) || (X < Y) --> X != MAX
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

define i1 @slt_or_not_max(i8 %x, i8 %y)  {
; CHECK-LABEL: @slt_or_not_max(
; CHECK-NEXT:    [[CMP:%.*]] = icmp slt i8 [[X:%.*]], [[Y:%.*]]
; CHECK-NEXT:    [[CMPEQ:%.*]] = icmp ne i8 [[X]], 127
; CHECK-NEXT:    [[R:%.*]] = or i1 [[CMP]], [[CMPEQ]]
; CHECK-NEXT:    ret i1 [[R]]
;
  %cmp = icmp slt i8 %x, %y
  %cmpeq = icmp ne i8 %x, 127
  %r = or i1 %cmp, %cmpeq
  ret i1 %r
}

define i1 @slt_or_not_max_commute(i8 %x, i8 %y)  {
; CHECK-LABEL: @slt_or_not_max_commute(
; CHECK-NEXT:    [[CMP:%.*]] = icmp slt i8 [[X:%.*]], [[Y:%.*]]
; CHECK-NEXT:    [[CMPEQ:%.*]] = icmp ne i8 [[X]], 127
; CHECK-NEXT:    [[R:%.*]] = or i1 [[CMPEQ]], [[CMP]]
; CHECK-NEXT:    ret i1 [[R]]
;
  %cmp = icmp slt i8 %x, %y
  %cmpeq = icmp ne i8 %x, 127
  %r = or i1 %cmpeq, %cmp
  ret i1 %r
}

define i1 @slt_swap_or_not_max(i8 %x, i8 %y)  {
; CHECK-LABEL: @slt_swap_or_not_max(
; CHECK-NEXT:    [[CMP:%.*]] = icmp sgt i8 [[Y:%.*]], [[X:%.*]]
; CHECK-NEXT:    [[CMPEQ:%.*]] = icmp ne i8 [[X]], 127
; CHECK-NEXT:    [[R:%.*]] = or i1 [[CMP]], [[CMPEQ]]
; CHECK-NEXT:    ret i1 [[R]]
;
  %cmp = icmp sgt i8 %y, %x
  %cmpeq = icmp ne i8 %x, 127
  %r = or i1 %cmp, %cmpeq
  ret i1 %r
}

define i1 @slt_swap_or_not_max_commute(i8 %x, i8 %y)  {
; CHECK-LABEL: @slt_swap_or_not_max_commute(
; CHECK-NEXT:    [[CMP:%.*]] = icmp sgt i8 [[Y:%.*]], [[X:%.*]]
; CHECK-NEXT:    [[CMPEQ:%.*]] = icmp ne i8 [[X]], 127
; CHECK-NEXT:    [[R:%.*]] = or i1 [[CMPEQ]], [[CMP]]
; CHECK-NEXT:    ret i1 [[R]]
;
  %cmp = icmp sgt i8 %y, %x
  %cmpeq = icmp ne i8 %x, 127
  %r = or i1 %cmpeq, %cmp
  ret i1 %r
}

define i1 @ult_or_not_max(i8 %x, i8 %y)  {
; CHECK-LABEL: @ult_or_not_max(
; CHECK-NEXT:    [[CMP:%.*]] = icmp ult i8 [[X:%.*]], [[Y:%.*]]
; CHECK-NEXT:    [[CMPEQ:%.*]] = icmp ne i8 [[X]], -1
; CHECK-NEXT:    [[R:%.*]] = or i1 [[CMP]], [[CMPEQ]]
; CHECK-NEXT:    ret i1 [[R]]
;
  %cmp = icmp ult i8 %x, %y
  %cmpeq = icmp ne i8 %x, 255
  %r = or i1 %cmp, %cmpeq
  ret i1 %r
}

define i1 @ult_or_not_max_commute(i8 %x, i8 %y)  {
; CHECK-LABEL: @ult_or_not_max_commute(
; CHECK-NEXT:    [[CMP:%.*]] = icmp ult i8 [[X:%.*]], [[Y:%.*]]
; CHECK-NEXT:    [[CMPEQ:%.*]] = icmp ne i8 [[X]], -1
; CHECK-NEXT:    [[R:%.*]] = or i1 [[CMPEQ]], [[CMP]]
; CHECK-NEXT:    ret i1 [[R]]
;
  %cmp = icmp ult i8 %x, %y
  %cmpeq = icmp ne i8 %x, 255
  %r = or i1 %cmpeq, %cmp
  ret i1 %r
}

define i1 @ult_swap_or_not_max(i8 %x, i8 %y)  {
; CHECK-LABEL: @ult_swap_or_not_max(
; CHECK-NEXT:    [[CMP:%.*]] = icmp ugt i8 [[Y:%.*]], [[X:%.*]]
; CHECK-NEXT:    [[CMPEQ:%.*]] = icmp ne i8 [[X]], -1
; CHECK-NEXT:    [[R:%.*]] = or i1 [[CMP]], [[CMPEQ]]
; CHECK-NEXT:    ret i1 [[R]]
;
  %cmp = icmp ugt i8 %y, %x
  %cmpeq = icmp ne i8 %x, 255
  %r = or i1 %cmp, %cmpeq
  ret i1 %r
}

define i1 @ult_swap_or_not_max_commute(i8 %x, i8 %y)  {
; CHECK-LABEL: @ult_swap_or_not_max_commute(
; CHECK-NEXT:    [[CMP:%.*]] = icmp ugt i8 [[Y:%.*]], [[X:%.*]]
; CHECK-NEXT:    [[CMPEQ:%.*]] = icmp ne i8 [[X]], -1
; CHECK-NEXT:    [[R:%.*]] = or i1 [[CMPEQ]], [[CMP]]
; CHECK-NEXT:    ret i1 [[R]]
;
  %cmp = icmp ugt i8 %y, %x
  %cmpeq = icmp ne i8 %x, 255
  %r = or i1 %cmpeq, %cmp
  ret i1 %r
}

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; (X != MIN) || (X > Y) --> X != MIN
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

define i1 @sgt_or_not_min(i8 %x, i8 %y)  {
; CHECK-LABEL: @sgt_or_not_min(
; CHECK-NEXT:    [[CMP:%.*]] = icmp sgt i8 [[X:%.*]], [[Y:%.*]]
; CHECK-NEXT:    [[CMPEQ:%.*]] = icmp ne i8 [[X]], -128
; CHECK-NEXT:    [[R:%.*]] = or i1 [[CMP]], [[CMPEQ]]
; CHECK-NEXT:    ret i1 [[R]]
;
  %cmp = icmp sgt i8 %x, %y
  %cmpeq = icmp ne i8 %x, 128
  %r = or i1 %cmp, %cmpeq
  ret i1 %r
}

define i1 @sgt_or_not_min_commute(i8 %x, i8 %y)  {
; CHECK-LABEL: @sgt_or_not_min_commute(
; CHECK-NEXT:    [[CMP:%.*]] = icmp sgt i8 [[X:%.*]], [[Y:%.*]]
; CHECK-NEXT:    [[CMPEQ:%.*]] = icmp ne i8 [[X]], -128
; CHECK-NEXT:    [[R:%.*]] = or i1 [[CMPEQ]], [[CMP]]
; CHECK-NEXT:    ret i1 [[R]]
;
  %cmp = icmp sgt i8 %x, %y
  %cmpeq = icmp ne i8 %x, 128
  %r = or i1 %cmpeq, %cmp
  ret i1 %r
}

define i1 @sgt_swap_or_not_min(i8 %x, i8 %y)  {
; CHECK-LABEL: @sgt_swap_or_not_min(
; CHECK-NEXT:    [[CMP:%.*]] = icmp slt i8 [[Y:%.*]], [[X:%.*]]
; CHECK-NEXT:    [[CMPEQ:%.*]] = icmp ne i8 [[X]], -128
; CHECK-NEXT:    [[R:%.*]] = or i1 [[CMP]], [[CMPEQ]]
; CHECK-NEXT:    ret i1 [[R]]
;
  %cmp = icmp slt i8 %y, %x
  %cmpeq = icmp ne i8 %x, 128
  %r = or i1 %cmp, %cmpeq
  ret i1 %r
}

define i1 @sgt_swap_or_not_min_commute(i8 %x, i8 %y)  {
; CHECK-LABEL: @sgt_swap_or_not_min_commute(
; CHECK-NEXT:    [[CMP:%.*]] = icmp slt i8 [[Y:%.*]], [[X:%.*]]
; CHECK-NEXT:    [[CMPEQ:%.*]] = icmp ne i8 [[X]], -128
; CHECK-NEXT:    [[R:%.*]] = or i1 [[CMPEQ]], [[CMP]]
; CHECK-NEXT:    ret i1 [[R]]
;
  %cmp = icmp slt i8 %y, %x
  %cmpeq = icmp ne i8 %x, 128
  %r = or i1 %cmpeq, %cmp
  ret i1 %r
}

define i1 @ugt_or_not_min(i8 %x, i8 %y)  {
; CHECK-LABEL: @ugt_or_not_min(
; CHECK-NEXT:    [[CMPEQ:%.*]] = icmp ne i8 [[X:%.*]], 0
; CHECK-NEXT:    ret i1 [[CMPEQ]]
;
  %cmp = icmp ugt i8 %x, %y
  %cmpeq = icmp ne i8 %x, 0
  %r = or i1 %cmp, %cmpeq
  ret i1 %r
}

define i1 @ugt_or_not_min_commute(i8 %x, i8 %y)  {
; CHECK-LABEL: @ugt_or_not_min_commute(
; CHECK-NEXT:    [[CMPEQ:%.*]] = icmp ne i8 [[X:%.*]], 0
; CHECK-NEXT:    ret i1 [[CMPEQ]]
;
  %cmp = icmp ugt i8 %x, %y
  %cmpeq = icmp ne i8 %x, 0
  %r = or i1 %cmpeq, %cmp
  ret i1 %r
}

define i1 @ugt_swap_or_not_min(i8 %x, i8 %y)  {
; CHECK-LABEL: @ugt_swap_or_not_min(
; CHECK-NEXT:    [[CMPEQ:%.*]] = icmp ne i8 [[X:%.*]], 0
; CHECK-NEXT:    ret i1 [[CMPEQ]]
;
  %cmp = icmp ult i8 %y, %x
  %cmpeq = icmp ne i8 %x, 0
  %r = or i1 %cmp, %cmpeq
  ret i1 %r
}

define i1 @ugt_swap_or_not_min_commute(i823 %x, i823 %y)  {
; CHECK-LABEL: @ugt_swap_or_not_min_commute(
; CHECK-NEXT:    [[CMPEQ:%.*]] = icmp ne i823 [[X:%.*]], 0
; CHECK-NEXT:    ret i1 [[CMPEQ]]
;
  %cmp = icmp ult i823 %y, %x
  %cmpeq = icmp ne i823 %x, 0
  %r = or i1 %cmpeq, %cmp
  ret i1 %r
}
