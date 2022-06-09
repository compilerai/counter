; NOTE: Assertions have been autogenerated by utils/update_test_checks.py
; RUN: opt < %s -ipconstprop -S | FileCheck %s

%0 = type { i32, i32 }

define internal %0 @foo(i1 %Q) {
; CHECK-LABEL: @foo(
; CHECK-NEXT:    br i1 [[Q:%.*]], label [[T:%.*]], label [[F:%.*]]
; CHECK:       T:
; CHECK-NEXT:    [[MRV:%.*]] = insertvalue [[TMP0:%.*]] undef, i32 21, 0
; CHECK-NEXT:    [[MRV1:%.*]] = insertvalue [[TMP0]] %mrv, i32 22, 1
; CHECK-NEXT:    ret [[TMP0]] %mrv1
; CHECK:       F:
; CHECK-NEXT:    [[MRV2:%.*]] = insertvalue [[TMP0]] undef, i32 21, 0
; CHECK-NEXT:    [[MRV3:%.*]] = insertvalue [[TMP0]] %mrv2, i32 23, 1
; CHECK-NEXT:    ret [[TMP0]] %mrv3
;
  br i1 %Q, label %T, label %F

T:                                                ; preds = %0
  %mrv = insertvalue %0 undef, i32 21, 0
  %mrv1 = insertvalue %0 %mrv, i32 22, 1
  ret %0 %mrv1

F:                                                ; preds = %0
  %mrv2 = insertvalue %0 undef, i32 21, 0
  %mrv3 = insertvalue %0 %mrv2, i32 23, 1
  ret %0 %mrv3
}

define internal %0 @bar(i1 %Q) {
; CHECK-LABEL: @bar(
; CHECK-NEXT:    [[A:%.*]] = insertvalue [[TMP0:%.*]] undef, i32 21, 0
; CHECK-NEXT:    br i1 [[Q:%.*]], label [[T:%.*]], label [[F:%.*]]
; CHECK:       T:
; CHECK-NEXT:    [[B:%.*]] = insertvalue [[TMP0]] %A, i32 22, 1
; CHECK-NEXT:    ret [[TMP0]] %B
; CHECK:       F:
; CHECK-NEXT:    [[C:%.*]] = insertvalue [[TMP0]] %A, i32 23, 1
; CHECK-NEXT:    ret [[TMP0]] %C
;
  %A = insertvalue %0 undef, i32 21, 0
  br i1 %Q, label %T, label %F

T:                                                ; preds = %0
  %B = insertvalue %0 %A, i32 22, 1
  ret %0 %B

F:                                                ; preds = %0
  %C = insertvalue %0 %A, i32 23, 1
  ret %0 %C
}

define %0 @caller(i1 %Q) {
; CHECK-LABEL: @caller(
; CHECK-NEXT:    [[X:%.*]] = call [[TMP0:%.*]] @foo(i1 [[Q:%.*]])
; CHECK-NEXT:    [[B:%.*]] = extractvalue [[TMP0]] %X, 1
; CHECK-NEXT:    [[Y:%.*]] = call [[TMP0]] @bar(i1 [[Q]])
; CHECK-NEXT:    [[D:%.*]] = extractvalue [[TMP0]] %Y, 1
; CHECK-NEXT:    [[M:%.*]] = add i32 21, 21
; CHECK-NEXT:    [[N:%.*]] = add i32 [[B]], [[D]]
; CHECK-NEXT:    ret [[TMP0]] %X
;
  %X = call %0 @foo(i1 %Q)
  %A = extractvalue %0 %X, 0
  %B = extractvalue %0 %X, 1
  %Y = call %0 @bar(i1 %Q)
  %C = extractvalue %0 %Y, 0
  %D = extractvalue %0 %Y, 1
  %M = add i32 %A, %C
  %N = add i32 %B, %D
  ret %0 %X
}
