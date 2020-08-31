--- tc-i386-intel.c	2015-05-22 03:11:38.895958828 +0530
+++ /home/sbansal/tmp/binutils-2.22/gas/config/tc-i386-intel.c	2010-11-03 19:48:41.000000000 +0530
@@ -193,7 +193,7 @@
 
   if (! strcmp (name, "$"))
     {
-      //current_location (e);
+      current_location (e);
       return 1;
     }
 
@@ -412,7 +412,7 @@
 
 	  if (ret && scale && (scale + 1))
 	    {
-	      //resolve_expression (scale);
+	      resolve_expression (scale);
 	      if (scale->X_op != O_constant
 		  || intel_state.index->reg_type.bitfield.reg16)
 		scale->X_add_number = 0;
@@ -728,7 +728,7 @@
 		expP->X_add_symbol = intel_state.seg;
 		i.op[this_operand].imms = expP;
 
-		//resolve_expression (expP);
+		resolve_expression (expP);
 		operand_type_set (&types, ~0);
 		if (!i386_finalize_immediate (S_GET_SEGMENT (intel_state.seg),
 					      expP, types, operand_string))
@@ -833,7 +833,7 @@
 
       expP = &disp_expressions[i.disp_operands];
       memcpy (expP, &exp, sizeof(exp));
-      //resolve_expression (expP);
+      resolve_expression (expP);
 
       if (expP->X_op != O_constant
 	  || expP->X_add_number
@@ -870,12 +870,10 @@
 	   * expert I can't really say whether that would have other bad side
 	   * effects.
 	   */
-    /*
 	  if (OUTPUT_FLAVOR == bfd_target_aout_flavour
 	      && exp_seg == reg_section)
 	    exp_seg = expP->X_op != O_constant ? undefined_section
 					       : absolute_section;
-                 */
 #endif
 
 	  if (!i386_finalize_displacement (exp_seg, expP,
