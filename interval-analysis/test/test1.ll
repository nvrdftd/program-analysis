; ModuleID = 'test1.c'
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

; Function Attrs: nounwind uwtable
define i32 @main() #0 {
  %1 = alloca i32, align 4
  %x = alloca i32, align 4
  %a = alloca i32, align 4
  %b = alloca i32, align 4
  store i32 0, i32* %1
  store i32 3, i32* %x, align 4
  store i32 -5, i32* %b, align 4
  %2 = load i32* %x, align 4
  %3 = load i32* %b, align 4
  %4 = add nsw i32 %2, %3
  %5 = load i32* %a, align 4
  %6 = sub nsw i32 %4, %5
  %7 = icmp sle i32 %6, 0
  br i1 %7, label %8, label %11

; <label>:8                                       ; preds = %0
  %9 = load i32* %b, align 4
  %10 = add nsw i32 3, %9
  store i32 %10, i32* %x, align 4
  br label %22

; <label>:11                                      ; preds = %0
  %12 = load i32* %a, align 4
  %13 = icmp sgt i32 %12, 0
  br i1 %13, label %14, label %17

; <label>:14                                      ; preds = %11
  %15 = load i32* %b, align 4
  %16 = sub nsw i32 %15, 10
  store i32 %16, i32* %x, align 4
  br label %21

; <label>:17                                      ; preds = %11
  store i32 3, i32* %b, align 4
  %18 = load i32* %b, align 4
  %19 = load i32* %x, align 4
  %20 = add nsw i32 %18, %19
  store i32 %20, i32* %x, align 4
  br label %21

; <label>:21                                      ; preds = %17, %14
  br label %22

; <label>:22                                      ; preds = %21, %8
  %23 = load i32* %x, align 4
  ret i32 %23
}

attributes #0 = { nounwind uwtable "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }

!llvm.ident = !{!0}

!0 = metadata !{metadata !"clang version 3.4.2 (tags/RELEASE_34/dot2-final)"}
