/* MIPS MXU2 intrinsics include file.

   Copyright (C) 2006 Ingenic Semiconductor CO.,LTD.
   Contributed by Qian Liu, qian.liu@ingenic.com.

   This file is part of GCC.

   GCC is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published
   by the Free Software Foundation; either version 3, or (at your
   option) any later version.

   GCC is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
   License for more details.

   Under Section 7 of GPL version 3, you are granted additional
   permissions described in the GCC Runtime Library Exception, version
   3.1, as published by the Free Software Foundation.

   You should have received a copy of the GNU General Public License and
   a copy of the GCC Runtime Library Exception along with this program;
   see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see
   <http://www.gnu.org/licenses/>.  */

#ifndef _MXU2_H
#define _MXU2_H 1

#if defined(__mips_mxu2)
typedef signed char v16i8 __attribute__((vector_size(16), aligned(16)));
typedef unsigned char v16u8 __attribute__((vector_size(16), aligned(16)));
typedef short v8i16 __attribute__((vector_size(16), aligned(16)));
typedef unsigned short v8u16 __attribute__((vector_size(16), aligned(16)));
typedef int v4i32 __attribute__((vector_size(16), aligned(16)));
typedef unsigned int v4u32 __attribute__((vector_size(16), aligned(16)));
typedef long long v2i64 __attribute__((vector_size(16), aligned(16)));
typedef unsigned long long v2u64 __attribute__((vector_size(16), aligned(16)));
typedef float v4f32 __attribute__((vector_size(16), aligned(16)));
typedef double v2f64 __attribute__ ((vector_size(16), aligned(16)));

/* The Intel API is flexible enough that we must allow aliasing with other
   vector types, and their scalar components.  */
typedef long long __m128i __attribute__ ((__vector_size__ (16), __may_alias__));
typedef double __m128d __attribute__ ((__vector_size__ (16), __may_alias__));
typedef float __m128 __attribute__ ((__vector_size__ (16), __may_alias__));



#ifndef __clang__
#define _mx128_bnez1q __builtin_mxu2_bnez1q
#define _mx128_bnez16b __builtin_mxu2_bnez16b
#define _mx128_bnez8h __builtin_mxu2_bnez8h
#define _mx128_bnez4w __builtin_mxu2_bnez4w
#define _mx128_bnez2d __builtin_mxu2_bnez2d
#define _mx128_beqz1q __builtin_mxu2_beqz1q
#define _mx128_beqz16b __builtin_mxu2_beqz16b
#define _mx128_beqz8h __builtin_mxu2_beqz8h
#define _mx128_beqz4w __builtin_mxu2_beqz4w
#define _mx128_beqz2d __builtin_mxu2_beqz2d
/* Compare */
#define _mx128_ceq_b __builtin_mxu2_ceq_b
#define _mx128_ceq_h __builtin_mxu2_ceq_h
#define _mx128_ceq_w __builtin_mxu2_ceq_w
#define _mx128_ceq_d __builtin_mxu2_ceq_d
#define _mx128_ceqz_b __builtin_mxu2_ceqz_b
#define _mx128_ceqz_h __builtin_mxu2_ceqz_h
#define _mx128_ceqz_w __builtin_mxu2_ceqz_w
#define _mx128_ceqz_d __builtin_mxu2_ceqz_d
#define _mx128_cne_b __builtin_mxu2_cne_b
#define _mx128_cne_h __builtin_mxu2_cne_h
#define _mx128_cne_w __builtin_mxu2_cne_w
#define _mx128_cne_d __builtin_mxu2_cne_d
#define _mx128_cnez_b __builtin_mxu2_cnez_b
#define _mx128_cnez_h __builtin_mxu2_cnez_h
#define _mx128_cnez_w __builtin_mxu2_cnez_w
#define _mx128_cnez_d __builtin_mxu2_cnez_d
#define _mx128_cles_b __builtin_mxu2_cles_b
#define _mx128_cles_h __builtin_mxu2_cles_h
#define _mx128_cles_w __builtin_mxu2_cles_w
#define _mx128_cles_d __builtin_mxu2_cles_d
#define _mx128_cleu_b __builtin_mxu2_cleu_b
#define _mx128_cleu_h __builtin_mxu2_cleu_h
#define _mx128_cleu_w __builtin_mxu2_cleu_w
#define _mx128_cleu_d __builtin_mxu2_cleu_d
#define _mx128_clez_b __builtin_mxu2_clez_b
#define _mx128_clez_h __builtin_mxu2_clez_h
#define _mx128_clez_w __builtin_mxu2_clez_w
#define _mx128_clez_d __builtin_mxu2_clez_d
#define _mx128_clts_b __builtin_mxu2_clts_b
#define _mx128_clts_h __builtin_mxu2_clts_h
#define _mx128_clts_w __builtin_mxu2_clts_w
#define _mx128_clts_d __builtin_mxu2_clts_d
#define _mx128_cltu_b __builtin_mxu2_cltu_b
#define _mx128_cltu_h __builtin_mxu2_cltu_h
#define _mx128_cltu_w __builtin_mxu2_cltu_w
#define _mx128_cltu_d __builtin_mxu2_cltu_d
#define _mx128_cltz_b __builtin_mxu2_cltz_b
#define _mx128_cltz_h __builtin_mxu2_cltz_h
#define _mx128_cltz_w __builtin_mxu2_cltz_w
#define _mx128_cltz_d __builtin_mxu2_cltz_d
/* Integer Arithmetic */
#define _mx128_adda_b __builtin_mxu2_adda_b
#define _mx128_adda_h __builtin_mxu2_adda_h
#define _mx128_adda_w __builtin_mxu2_adda_w
#define _mx128_adda_d __builtin_mxu2_adda_d
#define _mx128_addas_b __builtin_mxu2_addas_b
#define _mx128_addas_h __builtin_mxu2_addas_h
#define _mx128_addas_w __builtin_mxu2_addas_w
#define _mx128_addas_d __builtin_mxu2_addas_d
#define _mx128_addss_b __builtin_mxu2_addss_b
#define _mx128_addss_h __builtin_mxu2_addss_h
#define _mx128_addss_w __builtin_mxu2_addss_w
#define _mx128_addss_d __builtin_mxu2_addss_d
#define _mx128_adduu_b __builtin_mxu2_adduu_b
#define _mx128_adduu_h __builtin_mxu2_adduu_h
#define _mx128_adduu_w __builtin_mxu2_adduu_w
#define _mx128_adduu_d __builtin_mxu2_adduu_d
#define _mx128_add_b __builtin_mxu2_add_b
#define _mx128_add_h __builtin_mxu2_add_h
#define _mx128_add_w __builtin_mxu2_add_w
#define _mx128_add_d __builtin_mxu2_add_d
#define _mx128_subsa_b __builtin_mxu2_subsa_b
#define _mx128_subsa_h __builtin_mxu2_subsa_h
#define _mx128_subsa_w __builtin_mxu2_subsa_w
#define _mx128_subsa_d __builtin_mxu2_subsa_d
#define _mx128_subua_b __builtin_mxu2_subua_b
#define _mx128_subua_h __builtin_mxu2_subua_h
#define _mx128_subua_w __builtin_mxu2_subua_w
#define _mx128_subua_d __builtin_mxu2_subua_d
#define _mx128_subss_b __builtin_mxu2_subss_b
#define _mx128_subss_h __builtin_mxu2_subss_h
#define _mx128_subss_w __builtin_mxu2_subss_w
#define _mx128_subss_d __builtin_mxu2_subss_d
#define _mx128_subuu_b __builtin_mxu2_subuu_b
#define _mx128_subuu_h __builtin_mxu2_subuu_h
#define _mx128_subuu_w __builtin_mxu2_subuu_w
#define _mx128_subuu_d __builtin_mxu2_subuu_d
#define _mx128_subus_b __builtin_mxu2_subus_b
#define _mx128_subus_h __builtin_mxu2_subus_h
#define _mx128_subus_w __builtin_mxu2_subus_w
#define _mx128_subus_d __builtin_mxu2_subus_d
#define _mx128_sub_b __builtin_mxu2_sub_b
#define _mx128_sub_h __builtin_mxu2_sub_h
#define _mx128_sub_w __builtin_mxu2_sub_w
#define _mx128_sub_d __builtin_mxu2_sub_d
#define _mx128_aves_b __builtin_mxu2_aves_b
#define _mx128_aves_h __builtin_mxu2_aves_h
#define _mx128_aves_w __builtin_mxu2_aves_w
#define _mx128_aves_d __builtin_mxu2_aves_d
#define _mx128_aveu_b __builtin_mxu2_aveu_b
#define _mx128_aveu_h __builtin_mxu2_aveu_h
#define _mx128_aveu_w __builtin_mxu2_aveu_w
#define _mx128_aveu_d __builtin_mxu2_aveu_d
#define _mx128_avers_b __builtin_mxu2_avers_b
#define _mx128_avers_h __builtin_mxu2_avers_h
#define _mx128_avers_w __builtin_mxu2_avers_w
#define _mx128_avers_d __builtin_mxu2_avers_d
#define _mx128_averu_b __builtin_mxu2_averu_b
#define _mx128_averu_h __builtin_mxu2_averu_h
#define _mx128_averu_w __builtin_mxu2_averu_w
#define _mx128_averu_d __builtin_mxu2_averu_d
#define _mx128_divs_b __builtin_mxu2_divs_b
#define _mx128_divs_h __builtin_mxu2_divs_h
#define _mx128_divs_w __builtin_mxu2_divs_w
#define _mx128_divs_d __builtin_mxu2_divs_d
#define _mx128_divu_b __builtin_mxu2_divu_b
#define _mx128_divu_h __builtin_mxu2_divu_h
#define _mx128_divu_w __builtin_mxu2_divu_w
#define _mx128_divu_d __builtin_mxu2_divu_d
#define _mx128_mods_b __builtin_mxu2_mods_b
#define _mx128_mods_h __builtin_mxu2_mods_h
#define _mx128_mods_w __builtin_mxu2_mods_w
#define _mx128_mods_d __builtin_mxu2_mods_d
#define _mx128_modu_b __builtin_mxu2_modu_b
#define _mx128_modu_h __builtin_mxu2_modu_h
#define _mx128_modu_w __builtin_mxu2_modu_w
#define _mx128_modu_d __builtin_mxu2_modu_d
#define _mx128_madd_b __builtin_mxu2_madd_b
#define _mx128_madd_h __builtin_mxu2_madd_h
#define _mx128_madd_w __builtin_mxu2_madd_w
#define _mx128_madd_d __builtin_mxu2_madd_d
#define _mx128_msub_b __builtin_mxu2_msub_b
#define _mx128_msub_h __builtin_mxu2_msub_h
#define _mx128_msub_w __builtin_mxu2_msub_w
#define _mx128_msub_d __builtin_mxu2_msub_d
#define _mx128_mul_b __builtin_mxu2_mul_b
#define _mx128_mul_h __builtin_mxu2_mul_h
#define _mx128_mul_w __builtin_mxu2_mul_w
#define _mx128_mul_d __builtin_mxu2_mul_d
#define _mx128_maxa_b __builtin_mxu2_maxa_b
#define _mx128_maxa_h __builtin_mxu2_maxa_h
#define _mx128_maxa_w __builtin_mxu2_maxa_w
#define _mx128_maxa_d __builtin_mxu2_maxa_d
#define _mx128_maxs_b __builtin_mxu2_maxs_b
#define _mx128_maxs_h __builtin_mxu2_maxs_h
#define _mx128_maxs_w __builtin_mxu2_maxs_w
#define _mx128_maxs_d __builtin_mxu2_maxs_d
#define _mx128_maxu_b __builtin_mxu2_maxu_b
#define _mx128_maxu_h __builtin_mxu2_maxu_h
#define _mx128_maxu_w __builtin_mxu2_maxu_w
#define _mx128_maxu_d __builtin_mxu2_maxu_d
#define _mx128_mina_b __builtin_mxu2_mina_b
#define _mx128_mina_h __builtin_mxu2_mina_h
#define _mx128_mina_w __builtin_mxu2_mina_w
#define _mx128_mina_d __builtin_mxu2_mina_d
#define _mx128_mins_b __builtin_mxu2_mins_b
#define _mx128_mins_h __builtin_mxu2_mins_h
#define _mx128_mins_w __builtin_mxu2_mins_w
#define _mx128_mins_d __builtin_mxu2_mins_d
#define _mx128_minu_b __builtin_mxu2_minu_b
#define _mx128_minu_h __builtin_mxu2_minu_h
#define _mx128_minu_w __builtin_mxu2_minu_w
#define _mx128_minu_d __builtin_mxu2_minu_d
#define _mx128_sats_b __builtin_mxu2_sats_b
#define _mx128_sats_h __builtin_mxu2_sats_h
#define _mx128_sats_w __builtin_mxu2_sats_w
#define _mx128_sats_d __builtin_mxu2_sats_d
#define _mx128_satu_b __builtin_mxu2_satu_b
#define _mx128_satu_h __builtin_mxu2_satu_h
#define _mx128_satu_w __builtin_mxu2_satu_w
#define _mx128_satu_d __builtin_mxu2_satu_d
/* Dot */
#define _mx128_dotps_h __builtin_mxu2_dotps_h
#define _mx128_dotps_w __builtin_mxu2_dotps_w
#define _mx128_dotps_d __builtin_mxu2_dotps_d
#define _mx128_dotpu_h __builtin_mxu2_dotpu_h
#define _mx128_dotpu_w __builtin_mxu2_dotpu_w
#define _mx128_dotpu_d __builtin_mxu2_dotpu_d
#define _mx128_dadds_h __builtin_mxu2_dadds_h
#define _mx128_dadds_w __builtin_mxu2_dadds_w
#define _mx128_dadds_d __builtin_mxu2_dadds_d
#define _mx128_daddu_h __builtin_mxu2_daddu_h
#define _mx128_daddu_w __builtin_mxu2_daddu_w
#define _mx128_daddu_d __builtin_mxu2_daddu_d
#define _mx128_dsubs_h __builtin_mxu2_dsubs_h
#define _mx128_dsubs_w __builtin_mxu2_dsubs_w
#define _mx128_dsubs_d __builtin_mxu2_dsubs_d
#define _mx128_dsubu_h __builtin_mxu2_dsubu_h
#define _mx128_dsubu_w __builtin_mxu2_dsubu_w
#define _mx128_dsubu_d __builtin_mxu2_dsubu_d
/* Bitwise */
#define _mx128_loc_b __builtin_mxu2_loc_b
#define _mx128_loc_h __builtin_mxu2_loc_h
#define _mx128_loc_w __builtin_mxu2_loc_w
#define _mx128_loc_d __builtin_mxu2_loc_d
#define _mx128_lzc_b __builtin_mxu2_lzc_b
#define _mx128_lzc_h __builtin_mxu2_lzc_h
#define _mx128_lzc_w __builtin_mxu2_lzc_w
#define _mx128_lzc_d __builtin_mxu2_lzc_d
#define _mx128_bcnt_b __builtin_mxu2_bcnt_b
#define _mx128_bcnt_h __builtin_mxu2_bcnt_h
#define _mx128_bcnt_w __builtin_mxu2_bcnt_w
#define _mx128_bcnt_d __builtin_mxu2_bcnt_d
#define _mx128_andv __builtin_mxu2_andv
#define _mx128_andib __builtin_mxu2_andib
#define _mx128_norv __builtin_mxu2_norv
#define _mx128_norib __builtin_mxu2_norib
#define _mx128_orv __builtin_mxu2_orv
#define _mx128_orib __builtin_mxu2_orib
#define _mx128_xorv __builtin_mxu2_xorv
#define _mx128_xorib __builtin_mxu2_xorib
#define _mx128_bselv __builtin_mxu2_bselv
/* Float Point Arithmetic */
#define _mx128_fadd_w __builtin_mxu2_fadd_w
#define _mx128_fadd_d __builtin_mxu2_fadd_d
#define _mx128_fsub_w __builtin_mxu2_fsub_w
#define _mx128_fsub_d __builtin_mxu2_fsub_d
#define _mx128_fmul_w __builtin_mxu2_fmul_w
#define _mx128_fmul_d __builtin_mxu2_fmul_d
#define _mx128_fdiv_w __builtin_mxu2_fdiv_w
#define _mx128_fdiv_d __builtin_mxu2_fdiv_d
#define _mx128_fsqrt_w __builtin_mxu2_fsqrt_w
#define _mx128_fsqrt_d __builtin_mxu2_fsqrt_d
#define _mx128_fmadd_w __builtin_mxu2_fmadd_w
#define _mx128_fmadd_d __builtin_mxu2_fmadd_d
#define _mx128_fmsub_w __builtin_mxu2_fmsub_w
#define _mx128_fmsub_d __builtin_mxu2_fmsub_d
#define _mx128_fmax_w __builtin_mxu2_fmax_w
#define _mx128_fmax_d __builtin_mxu2_fmax_d
#define _mx128_fmaxa_w __builtin_mxu2_fmaxa_w
#define _mx128_fmaxa_d __builtin_mxu2_fmaxa_d
#define _mx128_fmin_w __builtin_mxu2_fmin_w
#define _mx128_fmin_d __builtin_mxu2_fmin_d
#define _mx128_fmina_w __builtin_mxu2_fmina_w
#define _mx128_fmina_d __builtin_mxu2_fmina_d
#define _mx128_fclass_w __builtin_mxu2_fclass_w
#define _mx128_fclass_d __builtin_mxu2_fclass_d
/* Float Point Compare */
#define _mx128_fceq_w __builtin_mxu2_fceq_w
#define _mx128_fceq_d __builtin_mxu2_fceq_d
#define _mx128_fcle_w __builtin_mxu2_fcle_w
#define _mx128_fcle_d __builtin_mxu2_fcle_d
#define _mx128_fclt_w __builtin_mxu2_fclt_w
#define _mx128_fclt_d __builtin_mxu2_fclt_d
#define _mx128_fcor_w __builtin_mxu2_fcor_w
#define _mx128_fcor_d __builtin_mxu2_fcor_d
/*Float Point Conversion */
#define _mx128_vcvths __builtin_mxu2_vcvths
#define _mx128_vcvtsd __builtin_mxu2_vcvtsd
#define _mx128_vcvtesh __builtin_mxu2_vcvtesh
#define _mx128_vcvteds __builtin_mxu2_vcvteds
#define _mx128_vcvtosh __builtin_mxu2_vcvtosh
#define _mx128_vcvtods __builtin_mxu2_vcvtods
#define _mx128_vcvtssw __builtin_mxu2_vcvtssw
#define _mx128_vcvtsdl __builtin_mxu2_vcvtsdl
#define _mx128_vcvtusw __builtin_mxu2_vcvtusw
#define _mx128_vcvtudl __builtin_mxu2_vcvtudl
#define _mx128_vcvtsws __builtin_mxu2_vcvtsws
#define _mx128_vcvtsld __builtin_mxu2_vcvtsld
#define _mx128_vcvtuws __builtin_mxu2_vcvtuws
#define _mx128_vcvtuld __builtin_mxu2_vcvtuld
#define _mx128_vcvtrws __builtin_mxu2_vcvtrws
#define _mx128_vcvtrld __builtin_mxu2_vcvtrld
#define _mx128_vtruncsws __builtin_mxu2_vtruncsws
#define _mx128_vtruncsld __builtin_mxu2_vtruncsld
#define _mx128_vtruncuws __builtin_mxu2_vtruncuws
#define _mx128_vtrunculd __builtin_mxu2_vtrunculd
#define _mx128_vcvtqesh __builtin_mxu2_vcvtqesh
#define _mx128_vcvtqedw __builtin_mxu2_vcvtqedw
#define _mx128_vcvtqosh __builtin_mxu2_vcvtqosh
#define _mx128_vcvtqodw __builtin_mxu2_vcvtqodw
#define _mx128_vcvtqhs __builtin_mxu2_vcvtqhs
#define _mx128_vcvtqwd __builtin_mxu2_vcvtqwd
/* Fixed Point Multiplication */
#define _mx128_maddq_h __builtin_mxu2_maddq_h
#define _mx128_maddq_w __builtin_mxu2_maddq_w
#define _mx128_maddqr_h __builtin_mxu2_maddqr_h
#define _mx128_maddqr_w __builtin_mxu2_maddqr_w
#define _mx128_msubq_h __builtin_mxu2_msubq_h
#define _mx128_msubq_w __builtin_mxu2_msubq_w
#define _mx128_msubqr_h __builtin_mxu2_msubqr_h
#define _mx128_msubqr_w __builtin_mxu2_msubqr_w
#define _mx128_mulq_h __builtin_mxu2_mulq_h
#define _mx128_mulq_w __builtin_mxu2_mulq_w
#define _mx128_mulqr_h __builtin_mxu2_mulqr_h
#define _mx128_mulqr_w __builtin_mxu2_mulqr_w
/* Shift */
#define _mx128_sll_b __builtin_mxu2_sll_b
#define _mx128_sll_h __builtin_mxu2_sll_h
#define _mx128_sll_w __builtin_mxu2_sll_w
#define _mx128_sll_d __builtin_mxu2_sll_d
#define _mx128_slli_b __builtin_mxu2_slli_b
#define _mx128_slli_h __builtin_mxu2_slli_h
#define _mx128_slli_w __builtin_mxu2_slli_w
#define _mx128_slli_d __builtin_mxu2_slli_d
#define _mx128_sra_b __builtin_mxu2_sra_b
#define _mx128_sra_h __builtin_mxu2_sra_h
#define _mx128_sra_w __builtin_mxu2_sra_w
#define _mx128_sra_d __builtin_mxu2_sra_d
#define _mx128_srai_b __builtin_mxu2_srai_b
#define _mx128_srai_h __builtin_mxu2_srai_h
#define _mx128_srai_w __builtin_mxu2_srai_w
#define _mx128_srai_d __builtin_mxu2_srai_d
#define _mx128_srar_b __builtin_mxu2_srar_b
#define _mx128_srar_h __builtin_mxu2_srar_h
#define _mx128_srar_w __builtin_mxu2_srar_w
#define _mx128_srar_d __builtin_mxu2_srar_d
#define _mx128_srari_b __builtin_mxu2_srari_b
#define _mx128_srari_h __builtin_mxu2_srari_h
#define _mx128_srari_w __builtin_mxu2_srari_w
#define _mx128_srari_d __builtin_mxu2_srari_d
#define _mx128_srl_b __builtin_mxu2_srl_b
#define _mx128_srl_h __builtin_mxu2_srl_h
#define _mx128_srl_w __builtin_mxu2_srl_w
#define _mx128_srl_d __builtin_mxu2_srl_d
#define _mx128_srli_b __builtin_mxu2_srli_b
#define _mx128_srli_h __builtin_mxu2_srli_h
#define _mx128_srli_w __builtin_mxu2_srli_w
#define _mx128_srli_d __builtin_mxu2_srli_d
#define _mx128_srlr_b __builtin_mxu2_srlr_b
#define _mx128_srlr_h __builtin_mxu2_srlr_h
#define _mx128_srlr_w __builtin_mxu2_srlr_w
#define _mx128_srlr_d __builtin_mxu2_srlr_d
#define _mx128_srlri_b __builtin_mxu2_srlri_b
#define _mx128_srlri_h __builtin_mxu2_srlri_h
#define _mx128_srlri_w __builtin_mxu2_srlri_w
#define _mx128_srlri_d __builtin_mxu2_srlri_d
/* Element Permute */
#define _mx128_shufv __builtin_mxu2_shufv
#define _mx128_insfcpu_b __builtin_mxu2_insfcpu_b
#define _mx128_insfcpu_h __builtin_mxu2_insfcpu_h
#define _mx128_insfcpu_w __builtin_mxu2_insfcpu_w
#define _mx128_insfcpu_d __builtin_mxu2_insfcpu_d
#define _mx128_insffpu_w __builtin_mxu2_insffpu_w
#define _mx128_insffpu_d __builtin_mxu2_insffpu_d
#define _mx128_insfmxu_b __builtin_mxu2_insfmxu_b
#define _mx128_insfmxu_h __builtin_mxu2_insfmxu_h
#define _mx128_insfmxu_w __builtin_mxu2_insfmxu_w
#define _mx128_insfmxu_d __builtin_mxu2_insfmxu_d
#define _mx128_repx_b __builtin_mxu2_repx_b
#define _mx128_repx_h __builtin_mxu2_repx_h
#define _mx128_repx_w __builtin_mxu2_repx_w
#define _mx128_repx_d __builtin_mxu2_repx_d
#define _mx128_repi_b __builtin_mxu2_repi_b
#define _mx128_repi_h __builtin_mxu2_repi_h
#define _mx128_repi_w __builtin_mxu2_repi_w
#define _mx128_repi_d __builtin_mxu2_repi_d
/* Load/Store */
#define _mx128_mtcpus_b __builtin_mxu2_mtcpus_b
#define _mx128_mtcpus_h __builtin_mxu2_mtcpus_h
#define _mx128_mtcpus_w __builtin_mxu2_mtcpus_w
#define _mx128_mtcpus_d __builtin_mxu2_mtcpus_d
#define _mx128_mtcpuu_b __builtin_mxu2_mtcpuu_b
#define _mx128_mtcpuu_h __builtin_mxu2_mtcpuu_h
#define _mx128_mtcpuu_w __builtin_mxu2_mtcpuu_w
#define _mx128_mtcpuu_d __builtin_mxu2_mtcpuu_d
#define _mx128_mfcpu_b __builtin_mxu2_mfcpu_b
#define _mx128_mfcpu_h __builtin_mxu2_mfcpu_h
#define _mx128_mfcpu_w __builtin_mxu2_mfcpu_w
#define _mx128_mfcpu_d __builtin_mxu2_mfcpu_d
#define _mx128_mtfpu_w __builtin_mxu2_mtfpu_w
#define _mx128_mtfpu_d __builtin_mxu2_mtfpu_d
#define _mx128_mffpu_w __builtin_mxu2_mffpu_w
#define _mx128_mffpu_d __builtin_mxu2_mffpu_d
#define _mx128_ctcmxu __builtin_mxu2_ctcmxu
#define _mx128_cfcmxu __builtin_mxu2_cfcmxu
#define _mx128_lu1q __builtin_mxu2_lu1q
#define _mx128_lu1qx __builtin_mxu2_lu1qx
#define _mx128_la1q __builtin_mxu2_la1q
#define _mx128_la1qx __builtin_mxu2_la1qx
#define _mx128_su1q __builtin_mxu2_su1q
#define _mx128_su1qx __builtin_mxu2_su1qx
#define _mx128_sa1q __builtin_mxu2_sa1q
#define _mx128_sa1qx __builtin_mxu2_sa1qx
#define _mx128_li_b __builtin_mxu2_li_b
#define _mx128_li_h __builtin_mxu2_li_h
#define _mx128_li_w __builtin_mxu2_li_w
#define _mx128_li_d __builtin_mxu2_li_d
#endif /* __clang__ */
#endif /* defined(__mips_mxu2) */
#endif /* _MXU2_H */
