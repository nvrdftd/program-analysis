; ModuleID = 'test3.c'
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

; Function Attrs: nounwind uwtable
define i32 @main() #0 {
  %1 = alloca i32, align 4
  %N = alloca i32, align 4
  %a = alloca i32, align 4
  %b = alloca i32, align 4
  %x = alloca i32, align 4
  %y = alloca i32, align 4
  %z = alloca i32, align 4
  %i = alloca i32, align 4
  store i32 0, i32* %1
  store i32 10, i32* %x, align 4
  store i32 2, i32* %y, align 4
  store i32 0, i32* %z, align 4
  store i32 0, i32* %i, align 4
  %2 = load i32* %i, align 4
  %3 = load i32* %N, align 4
  %4 = icmp slt i32 %2, %3
  br i1 %4, label %5, label %25

; <label>:5                                       ; preds = %0
  %6 = load i32* %x, align 4
  %7 = load i32* %y, align 4
  %8 = mul nsw i32 2, %7
  %9 = mul nsw i32 %8, 3
  %10 = load i32* %z, align 4
  %11 = mul nsw i32 %9, %10
  %12 = add nsw i32 %6, %11
  %13 = srem i32 %12, 3
  %14 = sub nsw i32 0, %13
  store i32 %14, i32* %x, align 4
  %15 = load i32* %x, align 4
  %16 = mul nsw i32 3, %15
  %17 = load i32* %y, align 4
  %18 = mul nsw i32 2, %17
  %19 = add nsw i32 %16, %18
  %20 = load i32* %z, align 4
  %21 = add nsw i32 %19, %20
  %22 = srem i32 %21, 11
  store i32 %22, i32* %y, align 4
  %23 = load i32* %z, align 4
  %24 = add nsw i32 %23, 1
  store i32 %24, i32* %z, align 4
  br label %25

; <label>:25                                      ; preds = %5, %0
  ret i32 0
}

attributes #0 = { nounwind uwtable "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }

!llvm.ident = !{!0}

!0 = metadata !{metadata !"clang version 3.4.2 (tags/RELEASE_34/dot2-final)"}
