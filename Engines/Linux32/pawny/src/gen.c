/*--------------------------------------------------------------------------
    Pawny 1.0, chess engine (source code).
    Copyright (C) 2009 - 2013 by Mincho Georgiev.
    
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
    
    contact: pawnychess@gmail.com 
    web: http://www.pawny.netii.net/
----------------------------------------------------------------------------*/

#include "data.h"
#include "inline.h"

#define move_store(m, i, f, t, tp, p)\
{ m[i].from = (uint8)(f);\
  m[i].to   = (uint8)(t);\
  m[i].type = (tp);\
  m[i].promoted = (p);\
  m[i].score = 0;\
  i++;\
}
#define prom_store_w(m, i, f, t, tp)\
{ move_store((m),(i),(f),(t),(tp),(WQ));\
  move_store((m),(i),(f),(t),(tp),(WR));\
  move_store((m),(i),(f),(t),(tp),(WB));\
  move_store((m),(i),(f),(t),(tp),(WN));\
}
#define prom_store_b(m, i, f, t, tp)\
{ move_store((m),(i),(f),(t),(tp),(BQ));\
  move_store((m),(i),(f),(t),(tp),(BR));\
  move_store((m),(i),(f),(t),(tp),(BB));\
  move_store((m),(i),(f),(t),(tp),(BN));\
}

static int move_gen_w(move_t *ms)
{ int from, to, count = 0;
  bitboard_t pcs,m,c;
  bitboard_t occ = pos->occ[OCC_B] | pos->occ[OCC_W];
	
  ///pawn moves:
  //promotions:
  pcs = (pos->occ[WP] & (0xFFULL << 48));	//pawns on 7th rank
  m = ((pcs << 8) & (~occ));
  while(m)
  { to = bitscanf(m);
    bitclear(m, to);
    if(!is_pinned_w((to-8), to))
      prom_store_w(ms, count, (to-8), to, PROM);
  }
  //cap/prom - left side:
  m = ((pcs & ~FMASK_A) << 7) & pos->occ[OCC_B];
  while(m)
  { to = bitscanf(m);
    bitclear(m, to);
    if(!is_pinned_w((to-7), to))
      prom_store_w(ms, count, (to-7), to, CAP|PROM);
  }
  //cap/prom - right side:
  m = ((pcs & ~FMASK_H) << 9) & pos->occ[OCC_B];
  while(m)
  { to = bitscanf(m);
    bitclear(m, to);
    if(!is_pinned_w((to-9), to))
      prom_store_w(ms, count, (to-9), to, CAP|PROM);
  }
	
  //pawn captures:
  pcs = (pos->occ[WP] & ~(0xFFULL << 48)); //pawns that are not on 7th rank
  //left side:
  m = ((pcs & ~FMASK_A) << 7) & pos->occ[OCC_B];
  while(m)
  { to = bitscanf(m);
    bitclear(m, to);
    if(!is_pinned_w((to-7), to))
      move_store(ms, count, (to-7), to, CAP, 0);
  }
  //right side:
  m = ((pcs & ~FMASK_H) << 9) & pos->occ[OCC_B];
  while(m)
  { to = bitscanf(m);
    bitclear(m, to);
    if(!is_pinned_w((to-9), to))
      move_store(ms, count, (to-9), to, CAP, 0);
  }
	
  //single pawn advancing:
  m = (pcs << 8) & (~occ);
  while(m)
  { to = bitscanf(m);
    bitclear(m, to);
    if(!is_pinned_w((to-8), to))
      move_store(ms, count, (to-8), to, 0, 0);
  }
  //double advancing:
  m = (pcs & (0xFFULL << 8)); //pawns on 2nd rank
  m = ((m << 8) & (~occ));
  m = ((m << 8) & (~occ));
  while(m)
  { to = bitscanf(m);
    bitclear(m, to);
    if(!is_pinned_w((to-16), to))
      move_store(ms, count, (to-16), to, 0, 0);
  }

  //e.p.:
  if(pos->ep && pos->square[pos->ep] == EMPTY
  && pos->square[pos->ep-8] == BP)
  { if(pos->square[pos->ep-7] == WP
    && (p_attacks[W][pos->ep-7] & (1ULL << pos->ep)))
    { if(!is_pinned_ep_w((pos->ep-7), pos->ep))
        move_store(ms, count, (pos->ep-7), pos->ep, (CAP|EP), 0);
    }
    if(pos->square[pos->ep-9] == WP
    && (p_attacks[W][pos->ep-9] & (1ULL << pos->ep)))
    { if(!is_pinned_ep_w((pos->ep-9), pos->ep))
        move_store(ms, count, (pos->ep-9), pos->ep, (CAP|EP), 0);
    }
  }
	
  ///knight moves 
  pcs = pos->occ[WN];
  while(pcs)
  { from = bitscanf(pcs);
    bitclear(pcs, from);
    //captures:
    m = n_moves[from] & pos->occ[OCC_B];
    while(m)
    { to = bitscanf(m);
      bitclear(m, to);
      if(!is_pinned_w(from, to))
        move_store(ms, count, from, to, CAP, 0);
    }
    //moves:
    m = n_moves[from] & ~(occ);
    while(m)
    { to = bitscanf(m);
      bitclear(m, to);
      if(!is_pinned_w(from, to))
        move_store(ms, count, from, to, 0, 0);
    }
  }
	
  ///king moves:
  from = bitscanf(pos->occ[WK]);
  //castling:
  if(from == E1)
  { if(pos->castle & WHITE_OO
    && pos->square[H1] == WR
    && !(occ & W_OO_MASK))
    { if(!is_sq_attacked_b(E1) && !is_sq_attacked_b(F1) && !is_sq_attacked_b(G1))
        move_store(ms, count, E1, G1,(CASTLE | WHITE_OO), 0);
    }
    if(pos->castle & WHITE_OOO
    && pos->square[A1] == WR
    && !(occ & W_OOO_MASK))
    { if(!is_sq_attacked_b(E1) && !is_sq_attacked_b(D1) && !is_sq_attacked_b(C1))
        move_store(ms, count, E1, C1, (CASTLE | WHITE_OOO), 0);
    }
  }
  //captures:
  m = k_moves[from] & pos->occ[OCC_B];
  while(m)
  { to = bitscanf(m);
    bitclear(m, to);
    if(!is_sq_attacked_b(to))
      move_store(ms, count, from, to, CAP, 0);
  }
  //moves:
  m = k_moves[from] & ~(occ);
  while(m)
  { to = bitscanf(m);
    bitclear(m, to);
    if(!is_sq_attacked_b(to))
      move_store(ms, count, from, to, 0, 0);
  }
	
  ///queen moves:
  pcs = pos->occ[WQ];
  while(pcs)
  { from = bitscanf(pcs);
    bitclear(pcs, from);
    m = bmoves(from, (occ)) | rmoves(from, (occ));
    m &= ~pos->occ[OCC_W];
    c = m & pos->occ[OCC_B];
    m &= ~pos->occ[OCC_B];
		
    //captures:
    while(c)
    { to = bitscanf(c);
      bitclear(c, to);
      if(!is_pinned_w(from, to))
        move_store(ms, count, from, to, CAP, 0);
    }
    //moves:
    while(m)
    { to = bitscanf(m);
      bitclear(m, to);
      if(!is_pinned_w(from, to))
        move_store(ms, count, from, to, 0, 0);
    }
  }
	
  ///rook moves:
  pcs = pos->occ[WR];
  while(pcs)
  { from = bitscanf(pcs);
    bitclear(pcs, from);
    m = rmoves(from, (occ));
    m &= ~pos->occ[OCC_W];
    c = m & pos->occ[OCC_B];
    m &= ~pos->occ[OCC_B];
		
    //captures:
    while(c)
    { to = bitscanf(c);
      bitclear(c, to);
      if(!is_pinned_w(from, to))
        move_store(ms, count, from, to, CAP, 0);
    }
    //moves:
    while(m)
    { to = bitscanf(m);
      bitclear(m, to);
      if(!is_pinned_w(from, to))
        move_store(ms, count, from, to, 0, 0);
    }
  }
	
  ///bishop moves:
  pcs = pos->occ[WB];
  while(pcs)
  { from = bitscanf(pcs);
    bitclear(pcs, from);
    m = bmoves(from, (occ));
    m &= ~pos->occ[OCC_W];
    c = m & pos->occ[OCC_B];
    m &= ~pos->occ[OCC_B];
		
    //captures:
    while(c)
    { to = bitscanf(c);
      bitclear(c, to);
      if(!is_pinned_w(from, to))
        move_store(ms, count, from, to, CAP, 0);
    }
    //moves:
    while(m)
    { to = bitscanf(m);
      bitclear(m, to);
      if(!is_pinned_w(from, to))
        move_store(ms, count, from, to, 0, 0);
    }
  }
	
  return count;
}

static int move_gen_b(move_t *ms)
{ int from, to, count = 0;
  bitboard_t pcs,m,c;
  bitboard_t occ = pos->occ[OCC_B] | pos->occ[OCC_W];	
	
  ///pawn moves:
  //promotions:
  pcs = (pos->occ[BP] & (0xFFULL << 8));	//pawns on 2nd rank
  m = ((pcs >> 8) & (~occ));
  while(m)
  { to = bitscanf(m);
    bitclear(m, to);
    if(!is_pinned_b((to+8), to))
      prom_store_b(ms, count, (to+8), to, PROM);
  }
  //cap/prom - right side:
  m = ((pcs & ~FMASK_H)>> 7) & pos->occ[OCC_W];
  while(m)
  { to = bitscanf(m);
    bitclear(m, to);
    if(!is_pinned_b((to+7), to))
      prom_store_b(ms, count, (to+7), to, CAP|PROM);
  }
  //cap/prom - left side:
  m = ((pcs & ~FMASK_A) >> 9) & pos->occ[OCC_W];
  while(m)
  { to = bitscanf(m);
    bitclear(m, to);
    if(!is_pinned_b((to+9), to))
      prom_store_b(ms, count, (to+9), to, CAP|PROM);
  }
	
  //pawn captures:
  pcs = (pos->occ[BP] & ~(0xFFULL << 8)); //pawns that are not on 2nd rank
  //right side:
  m = ((pcs & ~FMASK_H) >> 7) & pos->occ[OCC_W];
  while(m)
  { to = bitscanf(m);
    bitclear(m, to);
    if(!is_pinned_b((to+7), to))
      move_store(ms, count, (to+7), to, CAP, 0);
  }
  //left side:
  m = ((pcs & ~FMASK_A) >> 9) & pos->occ[OCC_W];
  while(m)
  { to = bitscanf(m);
    bitclear(m, to);
    if(!is_pinned_b((to+9), to))
      move_store(ms, count, (to+9), to, CAP, 0);
  }
	
  //single pawn advancing:
  m = (pcs >> 8) & (~occ);
  while(m)
  { to = bitscanf(m);
    bitclear(m, to);
    if(!is_pinned_b((to+8), to))
      move_store(ms, count, (to+8), to, 0, 0);
  }
  //double advancing:
  m = (pcs & (0xFFULL << 48)); //pawns on 7th rank
  m = ((m >> 8) & (~occ));
  m = ((m >> 8) & (~occ));
  while(m)
  { to = bitscanf(m);
    bitclear(m, to);
    if(!is_pinned_b((to+16), to))
      move_store(ms, count, (to+16), to, 0, 0);
  }
	
  //e.p.:
  if(pos->ep && pos->square[pos->ep] == EMPTY
  && pos->square[pos->ep+8] == WP)
  { if(pos->square[pos->ep+7] == BP
    && (p_attacks[B][pos->ep+7] & (1ULL << pos->ep)))
    { if(!is_pinned_ep_b((pos->ep+7), pos->ep))
        move_store(ms, count, (pos->ep+7), pos->ep, (CAP|EP), 0);
    }
    if(pos->square[pos->ep+9] == BP
    && (p_attacks[B][pos->ep+9] & (1ULL << pos->ep)))
    { if(!is_pinned_ep_b((pos->ep+9), pos->ep))
        move_store(ms, count, (pos->ep+9), pos->ep, (CAP|EP), 0);
    }
  }
	
  ///knight moves 
  pcs = pos->occ[BN];
  while(pcs)
  { from = bitscanf(pcs);
    bitclear(pcs, from);
    //captures:
    m = n_moves[from] & pos->occ[OCC_W];
    while(m)
    { to = bitscanf(m);
      bitclear(m, to);
      if(!is_pinned_b(from, to))
        move_store(ms, count, from, to, CAP, 0);
    }
    //moves:
    m = n_moves[from] & ~(occ);
    while(m)
    { to = bitscanf(m);
      bitclear(m, to);
      if(!is_pinned_b(from, to))
        move_store(ms, count, from, to, 0, 0);
    }
  }
	
  ///king moves:
  from = bitscanf(pos->occ[BK]);
  //castling:
  if(from == E8)
  { if(pos->castle & BLACK_OO
    && pos->square[H8] == BR
    && !(occ & B_OO_MASK))
    { if(!is_sq_attacked_w(E8) && !is_sq_attacked_w(F8) && !is_sq_attacked_w(G8))
        move_store(ms, count, E8, G8,(CASTLE | BLACK_OO), 0);
    }
    if(pos->castle & BLACK_OOO
    && pos->square[A8] == BR
    && !(occ & B_OOO_MASK))
    { if(!is_sq_attacked_w(E8) && !is_sq_attacked_w(D8) && !is_sq_attacked_w(C8))
        move_store(ms, count, E8, C8, (CASTLE | BLACK_OOO), 0);
    }
  }
  //captures:
  m = k_moves[from] & pos->occ[OCC_W];
  while(m)
  { to = bitscanf(m);
    bitclear(m, to);
    if(!is_sq_attacked_w(to))
      move_store(ms, count, from, to, CAP, 0);
  }
  //moves:
  m = k_moves[from] & ~(occ);
  while(m)
  { to = bitscanf(m);
    bitclear(m, to);
    if(!is_sq_attacked_w(to))
      move_store(ms, count, from, to, 0, 0);
  }
	
  ///queen moves:
  pcs = pos->occ[BQ];
  while(pcs)
  { from = bitscanf(pcs);
    bitclear(pcs, from);
    m = bmoves(from, (occ)) | rmoves(from, (occ));
    m &= ~pos->occ[OCC_B];
    c = m & pos->occ[OCC_W];
    m &= ~pos->occ[OCC_W];
		
    //captures:
    while(c)
    { to = bitscanf(c);
      bitclear(c, to);
      if(!is_pinned_b(from, to))
        move_store(ms, count, from, to, CAP, 0);
    }
    //moves:
    while(m)
    { to = bitscanf(m);
      bitclear(m, to);
      if(!is_pinned_b(from, to))
        move_store(ms, count, from, to, 0, 0);
    }
  }
	
  ///rook moves:
  pcs = pos->occ[BR];
  while(pcs)
  { from = bitscanf(pcs);
    bitclear(pcs, from);
    m = rmoves(from, (occ));
    m &= ~pos->occ[OCC_B];
    c = m & pos->occ[OCC_W];
    m &= ~pos->occ[OCC_W];
		
    //captures:
    while(c)
    { to = bitscanf(c);
      bitclear(c, to);
      if(!is_pinned_b(from, to))
        move_store(ms, count, from, to, CAP, 0);
    }
    //moves:
    while(m)
    { to = bitscanf(m);
      bitclear(m, to);
      if(!is_pinned_b(from, to))
        move_store(ms, count, from, to, 0, 0);
    }
  }
	
  ///bishop moves:
  pcs = pos->occ[BB];
  while(pcs)
  { from = bitscanf(pcs);
    bitclear(pcs, from);
    m = bmoves(from, (occ));
    m &= ~pos->occ[OCC_B];
    c = m & pos->occ[OCC_W];
    m &= ~pos->occ[OCC_W];
		
    //captures:
    while(c)
    { to = bitscanf(c);
      bitclear(c, to);
      if(!is_pinned_b(from, to))
        move_store(ms, count, from, to, CAP, 0);
    }
    //moves:
    while(m)
    { to = bitscanf(m);
      bitclear(m, to);
      if(!is_pinned_b(from, to))
        move_store(ms, count, from, to, 0, 0);
    }
  }
	
  return count;
}

static int king_evasions_w(move_t *ms, bitboard_t across)
{	
  bitboard_t m;
  int to, from = Ksq(W);
  int count = 0;
	
  //captures:
  m = (k_moves[from] & pos->occ[OCC_B]) & ~across;
  while(m)
  { to = bitscanf(m);
    bitclear(m, to);
    if(!is_sq_attacked_b(to))
      move_store(ms, count, from, to, CAP, 0);
  }
  //moves:
  m = (k_moves[from] & ~(pos->occ[OCC_B] | pos->occ[OCC_W])) & ~across;
  while(m)
  { to = bitscanf(m);
    bitclear(m, to);
    if(!is_sq_attacked_b(to))
      move_store(ms, count, from, to, 0, 0);
  }
	
  return count;
}

static int king_evasions_b(move_t *ms, bitboard_t across)
{
  bitboard_t m;
  int to, from = Ksq(B);
  int count = 0;
	
  //captures:
  m = (k_moves[from] & pos->occ[OCC_W]) & ~across;
  while(m)
  { to = bitscanf(m);
    bitclear(m, to);
    if(!is_sq_attacked_w(to))
      move_store(ms, count, from, to, CAP, 0);
  }
  //moves:
  m = (k_moves[from] & ~(pos->occ[OCC_B] | pos->occ[OCC_W])) & ~across;
  while(m)
  { to = bitscanf(m);
    bitclear(m, to);
    if(!is_sq_attacked_w(to))
      move_store(ms, count, from, to, 0, 0);
  }
	
  return count;
}

static int full_evasions_w(move_t *ms, bitboard_t attackers, bitboard_t front, bitboard_t across)
{
  int x, to;
  bitboard_t pcs,m;
  bitboard_t occ = pos->occ[OCC_B] | pos->occ[OCC_W];
  int from = Ksq(W);
  int count = 0;
	
  ///king evasions:
  //captures:
  m = (k_moves[from] & pos->occ[OCC_B]) & ~across;
  while(m)
  { to = bitscanf(m);
    bitclear(m, to);
    if(!is_sq_attacked_b(to))
      move_store(ms, count, from, to, CAP, 0);
  }
  //moves:
  m = (k_moves[from] & ~(occ)) & ~across;
  while(m)
  { to = bitscanf(m);
    bitclear(m, to);
    if(!is_sq_attacked_b(to))
      move_store(ms, count, from, to, 0, 0);
  }
	
  ///pawns:
  pcs = pos->occ[WP];
  if(Rank(bitscanf(attackers)) == RANK_8)
  { //cap/prom left
    m = ((pcs & ~FMASK_A) << 7) & attackers;
    if(m)
    { to = bitscanf(m);
      if(!is_pinned_w((to-7), to))
        prom_store_w(ms, count, (to-7), to, CAP|PROM);
    }
    //cap/prom right
    m = ((pcs & ~FMASK_H) << 9) & attackers;
    if(m)
    { to = bitscanf(m);
      if(!is_pinned_w((to-9), to))
        prom_store_w(ms, count, (to-9), to, CAP|PROM);
    }
  }
  else
  { //capture-left:
    m = ((pcs & ~FMASK_A) << 7) & attackers;
    if(m)
    { to = bitscanf(m);
      bitclear(m, to);
      if(!is_pinned_w((to-7), to))
        move_store(ms, count, (to-7), to, CAP, 0);
    }
    //capture-right;
    m = ((pcs & ~FMASK_H) << 9) & attackers;
    if(m)
    { to = bitscanf(m);
      bitclear(m, to);
      if(!is_pinned_w((to-9), to))
        move_store(ms, count, (to-9), to, CAP, 0);
    }
  }
  //covering single push:
  m = (pcs << 8) & front;
  while(m)
  { to = bitscanf(m);
    bitclear(m, to);
    if(!is_pinned_w((to-8), to))
    { if(Rank(to) == RANK_8)
        {prom_store_w(ms, count, (to-8), to, PROM);}
      else {move_store(ms, count, (to-8), to, 0, 0);}
    }
  }
  //covering double push:
  m = (pcs & (0xFFULL << 8)); //pawns on 2nd rank
  m = ((m << 8) & (~occ));
  m = ((m << 8) & (~occ));
  m &= front;
  while(m)
  { to = bitscanf(m);
    bitclear(m, to);
    if(!is_pinned_w((to-16), to))
      move_store(ms, count, (to-16), to, 0, 0);
  }
	
  ///queens:
  pcs = pos->occ[WQ];
  while(pcs)
  { x = bitscanf(pcs);
    bitclear(pcs, x);
    m = bmoves(x, occ) & attackers;
    if(m)
    { to = bitscanf(m);
      bitclear(m, to);
      if(!is_pinned_w(x, to))
        move_store(ms, count, x, to, CAP, 0);
    }
    m = bmoves(x, occ) & front;
    while(m)
    { to = bitscanf(m);
      bitclear(m, to);
      if(!is_pinned_w(x, to))
        move_store(ms, count, x, to, 0, 0);
    }
    m = rmoves(x, occ) & attackers;
    if(m)
    { to = bitscanf(m);
      bitclear(m, to);
      if(!is_pinned_w(x, to))
        move_store(ms, count, x, to, CAP, 0);
    }
    m = rmoves(x, occ) & front;
    while(m)
    { to = bitscanf(m);
      bitclear(m, to);
      if(!is_pinned_w(x, to))
        move_store(ms, count, x, to, 0, 0);
    }
  }
	
  ///rooks:
  pcs = pos->occ[WR];
  while(pcs)
  { x = bitscanf(pcs);
    bitclear(pcs, x);
    m = rmoves(x, occ) & attackers;
    if(m)
    { to = bitscanf(m);
      bitclear(m, to);
      if(!is_pinned_w(x, to))
        move_store(ms, count, x, to, CAP, 0);
    }
    m = rmoves(x, occ) & front;
    while(m)
    { to = bitscanf(m);
      bitclear(m, to);
      if(!is_pinned_w(x, to))
        move_store(ms, count, x, to, 0, 0);
    }
  }
	
  ///bishops
  pcs = pos->occ[WB];
  while(pcs)
  { x = bitscanf(pcs);
    bitclear(pcs, x);
    m = bmoves(x, occ) & attackers;
    if(m)
    { to = bitscanf(m);
      bitclear(m, to);
      if(!is_pinned_w(x, to))
        move_store(ms, count, x, to, CAP, 0);
    }
    m = bmoves(x, occ) & front;
    while(m)
    { to = bitscanf(m);
      bitclear(m, to);
      if(!is_pinned_w(x, to))
        move_store(ms, count, x, to, 0, 0);
    }
  }
	
  ///knights
  pcs = pos->occ[WN];
  while(pcs)
  { from = bitscanf(pcs);
    bitclear(pcs, from);
    //captures:
    m = n_moves[from] & attackers;
    if(m)
    { to = bitscanf(m);
      bitclear(m, to);
      if(!is_pinned_w(from, to))
        move_store(ms, count, from, to, CAP, 0);
    }
    //moves:
    m = n_moves[from] & front;
    while(m)
    { to = bitscanf(m);
      bitclear(m, to);
      if(!is_pinned_w(from, to))
        move_store(ms, count, from, to, 0, 0);
    }
  }
	
  ///e.p ?
  if((pos->ep && ((attackers & pos->occ[BP])))
  || (front & (1ULL << pos->ep)))
  { if(pos->square[pos->ep] == EMPTY
    && pos->square[pos->ep-8] == BP)
    { if(pos->square[pos->ep-7] == WP
      && (p_attacks[W][pos->ep-7] & (1ULL << pos->ep)))
      { if(!is_pinned_ep_w((pos->ep-7), pos->ep))
          move_store(ms, count, (pos->ep-7), pos->ep, (CAP|EP), 0);
      }
      if(pos->square[pos->ep-9] == WP
      && (p_attacks[W][pos->ep-9] & (1ULL << pos->ep)))
      { if(!is_pinned_ep_w((pos->ep-9), pos->ep))
          move_store(ms, count, (pos->ep-9), pos->ep, (CAP|EP), 0);
      }
    }
  }
	
  return count;
}

static int full_evasions_b(move_t *ms, bitboard_t attackers, bitboard_t front, bitboard_t across)
{
  int x, to;
  bitboard_t pcs,m;
  bitboard_t occ = pos->occ[OCC_B] | pos->occ[OCC_W];
  int from = Ksq(B);
  int count = 0;

  ///king evasions:
  //captures:
  m = (k_moves[from] & pos->occ[OCC_W]) & ~across;
  while(m)
  { to = bitscanf(m);
    bitclear(m, to);
    if(!is_sq_attacked_w(to))
      move_store(ms, count, from, to, CAP, 0);
  }
  //moves:
  m = (k_moves[from] & ~(occ)) & ~across;
  while(m)
  { to = bitscanf(m);
    bitclear(m, to);
    if(!is_sq_attacked_w(to))
      move_store(ms, count, from, to, 0, 0);
  }

  ///pawns:
  pcs = pos->occ[BP];
  if(Rank(bitscanf(attackers)) == RANK_1)
  { //cap/prom left
    m = ((pcs & ~FMASK_A) >> 9) & attackers;
    if(m)
    { to = bitscanf(m);
      if(!is_pinned_b((to+9), to))
        prom_store_b(ms, count, (to+9), to, CAP|PROM);
    }
    //cap/prom right
    m = ((pcs & ~FMASK_H) >> 7) & attackers;
    if(m)
    { to = bitscanf(m);
      if(!is_pinned_b((to+7), to))
        prom_store_b(ms, count, (to+7), to, CAP|PROM);
    }
  }
  else
  { //capture-left:
    m = ((pcs & ~FMASK_A) >> 9) & attackers;
    if(m)
    { to = bitscanf(m);
      bitclear(m, to);
      if(!is_pinned_b((to+9), to))
        move_store(ms, count, (to+9), to, CAP, 0);
    }
    //capture-right;
    m = ((pcs & ~FMASK_H) >> 7) & attackers;
    if(m)
    { to = bitscanf(m);
      bitclear(m, to);
      if(!is_pinned_b((to+7), to))
        move_store(ms, count, (to+7), to, CAP, 0);
    }
  }
  //covering single push:
  m = (pcs >> 8) & front;
  while(m)
  { to = bitscanf(m);
    bitclear(m, to);
    if(!is_pinned_b((to+8), to))
    { if(Rank(to) == RANK_1)
      {prom_store_b(ms, count, (to+8), to, PROM);}
      else{move_store(ms, count, (to+8), to, 0, 0);}
    }
  }
  //covering double push:
  m = (pcs & (0xFFULL << 48)); //pawns on 7th rank
  m = ((m >> 8) & (~occ));
  m = ((m >> 8) & (~occ));
  m &= front;
  while(m)
  { to = bitscanf(m);
    bitclear(m, to);
    if(!is_pinned_b((to+16), to))
    move_store(ms, count, (to+16), to, 0, 0);
  }

  ///queens:
  pcs = pos->occ[BQ];
  while(pcs)
  { x = bitscanf(pcs);
    bitclear(pcs, x);
    m = bmoves(x, occ) & attackers;
    if(m)
    { to = bitscanf(m);
      bitclear(m, to);
      if(!is_pinned_b(x, to))
        move_store(ms, count, x, to, CAP, 0);
    }
    m = bmoves(x, occ) & front;
    while(m)
    { to = bitscanf(m);
      bitclear(m, to);
      if(!is_pinned_b(x, to))
        move_store(ms, count, x, to, 0, 0);
    }
    m = rmoves(x, occ) & attackers;
    if(m)
    { to = bitscanf(m);
      bitclear(m, to);
      if(!is_pinned_b(x, to))
        move_store(ms, count, x, to, CAP, 0);
    }
    m = rmoves(x, occ) & front;
    while(m)
    { to = bitscanf(m);
      bitclear(m, to);
      if(!is_pinned_b(x, to))
        move_store(ms, count, x, to, 0, 0);
    }
  }

  ///rooks:
  pcs = pos->occ[BR];
  while(pcs)
  { x = bitscanf(pcs);
    bitclear(pcs, x);
    m = rmoves(x, occ) & attackers;
    if(m)
    { to = bitscanf(m);
      bitclear(m, to);
      if(!is_pinned_b(x, to))
        move_store(ms, count, x, to, CAP, 0);
    }
    m = rmoves(x, occ) & front;
    while(m)
    { to = bitscanf(m);
      bitclear(m, to);
      if(!is_pinned_b(x, to))
        move_store(ms, count, x, to, 0, 0);
    }
  }

  ///bishops
  pcs = pos->occ[BB];
  while(pcs)
  { x = bitscanf(pcs);
    bitclear(pcs, x);
    m = bmoves(x, occ) & attackers;
    if(m)
    { to = bitscanf(m);
      bitclear(m, to);
      if(!is_pinned_b(x, to))
        move_store(ms, count, x, to, CAP, 0);
    }
    m = bmoves(x, occ) & front;
    while(m)
    { to = bitscanf(m);
      bitclear(m, to);
      if(!is_pinned_b(x, to))
        move_store(ms, count, x, to, 0, 0);
    }
  }

  ///knights
  pcs = pos->occ[BN];
  while(pcs)
  { from = bitscanf(pcs);
    bitclear(pcs, from);
    //captures:
    m = n_moves[from] & attackers;
    if(m)
    { to = bitscanf(m);
      bitclear(m, to);
      if(!is_pinned_b(from, to))
        move_store(ms, count, from, to, CAP, 0);
    }
    //moves:
    m = n_moves[from] & front;
    while(m)
    { to = bitscanf(m);
      bitclear(m, to);
      if(!is_pinned_b(from, to))
        move_store(ms, count, from, to, 0, 0);
    }
  }

  //e.p.
  if((pos->ep && (attackers & pos->occ[WP]))
  || (front & (1ULL << pos->ep)))
  { if(pos->square[pos->ep] == EMPTY
    && pos->square[pos->ep+8] == WP)
    { if(pos->square[pos->ep+7] == BP
      && (p_attacks[B][pos->ep+7] & (1ULL << pos->ep)))
      { if(!is_pinned_ep_b((pos->ep+7), pos->ep))
          move_store(ms, count, (pos->ep+7), pos->ep, (CAP|EP), 0);
      }
      if(pos->square[pos->ep+9] == BP
      && (p_attacks[B][pos->ep+9] & (1ULL << pos->ep)))
      { if(!is_pinned_ep_b((pos->ep+9), pos->ep))
          move_store(ms, count, (pos->ep+9), pos->ep, (CAP|EP), 0);
      }
    }
  }
  return count;
}

int move_gen(move_t *ms)
{	
  int checks;
  bitboard_t attackers, front, across;

  if(pos->side==W) 
  { if(is_in_check_w())
    { checks = checkers_b(&attackers, &front, &across);
      if(checks >= 2) return king_evasions_w(ms, across);
      else return full_evasions_w(ms, attackers, front, across);
    }
    else return move_gen_w(ms);
  }
  else 
  { if(is_in_check_b())
    { checks = checkers_w(&attackers, &front, &across);
      if(checks >= 2) return king_evasions_b(ms, across);
      else return full_evasions_b(ms, attackers, front, across);
    }
    else return move_gen_b(ms);
  }
}

///captures:
int move_gen_caps_w(move_t *ms)
{
  int from, to, count = 0;
  bitboard_t pcs,m,c;
  bitboard_t occ = pos->occ[OCC_B] | pos->occ[OCC_W];

  ///pawn moves:
  //promotions:
  pcs = (pos->occ[WP] & (0xFFULL << 48));	//pawns on 7th rank
  m = ((pcs << 8) & (~occ));
  while(m)
  { to = bitscanf(m);
    bitclear(m, to);
    if(!is_pinned_w((to-8), to))
      prom_store_w(ms, count, (to-8), to, PROM);
  }
  //cap/prom - left side:
  m = ((pcs & ~FMASK_A) << 7) & pos->occ[OCC_B];
  while(m)
  { to = bitscanf(m);
    bitclear(m, to);
    if(!is_pinned_w((to-7), to))
      prom_store_w(ms, count, (to-7), to, CAP|PROM);
  }
  //cap/prom - right side:
  m = ((pcs & ~FMASK_H) << 9) & pos->occ[OCC_B];
  while(m)
  { to = bitscanf(m);
    bitclear(m, to);
    if(!is_pinned_w((to-9), to))
      prom_store_w(ms, count, (to-9), to, CAP|PROM);
  }
	
  //pawn captures:
  pcs = (pos->occ[WP] & ~(0xFFULL << 48)); //pawns that are not on 7th rank
  //left side:
  m = ((pcs & ~FMASK_A) << 7) & pos->occ[OCC_B];
  while(m)
  { to = bitscanf(m);
    bitclear(m, to);
    if(!is_pinned_w((to-7), to))
      move_store(ms, count, (to-7), to, CAP, 0);
  }
  //right side:
  m = ((pcs & ~FMASK_H) << 9) & pos->occ[OCC_B];
  while(m)
  { to = bitscanf(m);
    bitclear(m, to);
    if(!is_pinned_w((to-9), to))
      move_store(ms, count, (to-9), to, CAP, 0);
  }
	
  //e.p.:
  if(pos->ep && pos->square[pos->ep] == EMPTY
  && pos->square[pos->ep-8] == BP)
  { if(pos->square[pos->ep-7] == WP
    && (p_attacks[W][pos->ep-7] & (1ULL << pos->ep)))
    { if(!is_pinned_ep_w((pos->ep-7), pos->ep))
        move_store(ms, count, (pos->ep-7), pos->ep, (CAP|EP), 0);
    }
    if(pos->square[pos->ep-9] == WP
    && (p_attacks[W][pos->ep-9] & (1ULL << pos->ep)))
    { if(!is_pinned_ep_w((pos->ep-9), pos->ep))
        move_store(ms, count, (pos->ep-9), pos->ep, (CAP|EP), 0);
    }
  }
	
  ///knight moves 
  pcs = pos->occ[WN];
  while(pcs)
  { from = bitscanf(pcs);
    bitclear(pcs, from);
    //captures:
    m = n_moves[from] & pos->occ[OCC_B];
    while(m)
    { to = bitscanf(m);
      bitclear(m, to);
      if(!is_pinned_w(from, to))
        move_store(ms, count, from, to, CAP, 0);
    }
  }
	
  ///king moves:
  from = bitscanf(pos->occ[WK]);
  //captures:
  m = k_moves[from] & pos->occ[OCC_B];
  while(m)
  { to = bitscanf(m);
    bitclear(m, to);
    if(!is_sq_attacked_b(to))
      move_store(ms, count, from, to, CAP, 0);
  }
	
  ///queen moves:
  pcs = pos->occ[WQ];
  while(pcs)
  { from = bitscanf(pcs);
    bitclear(pcs, from);
    m = bmoves(from, (occ)) | rmoves(from, (occ));
    m &= ~pos->occ[OCC_W];
    c = m & pos->occ[OCC_B];
    m &= ~pos->occ[OCC_B];

    //captures:
    while(c)
    { to = bitscanf(c);
      bitclear(c, to);
      if(!is_pinned_w(from, to))
        move_store(ms, count, from, to, CAP, 0);
    }
  }
	
  ///rook moves:
  pcs = pos->occ[WR];
  while(pcs)
  { from = bitscanf(pcs);
    bitclear(pcs, from);
    m = rmoves(from, (occ));
    m &= ~pos->occ[OCC_W];
    c = m & pos->occ[OCC_B];
    m &= ~pos->occ[OCC_B];

    //captures:
    while(c)
    { to = bitscanf(c);
      bitclear(c, to);
      if(!is_pinned_w(from, to))
        move_store(ms, count, from, to, CAP, 0);
    }
  }
	
  ///bishop moves:
  pcs = pos->occ[WB];
  while(pcs)
  { from = bitscanf(pcs);
    bitclear(pcs, from);
    m = bmoves(from, (occ));
    m &= ~pos->occ[OCC_W];
    c = m & pos->occ[OCC_B];
    m &= ~pos->occ[OCC_B];

    //captures:
    while(c)
    { to = bitscanf(c);
      bitclear(c, to);
      if(!is_pinned_w(from, to))
        move_store(ms, count, from, to, CAP, 0);
    }
  }
  return count;
}

int move_gen_caps_b(move_t *ms)
{
  int from, to, count = 0;
  bitboard_t pcs,m,c;
  bitboard_t occ = pos->occ[OCC_B] | pos->occ[OCC_W];	
	
  ///pawn moves:
  //promotions:
  pcs = (pos->occ[BP] & (0xFFULL << 8));	//pawns on 2nd rank
  m = ((pcs >> 8) & (~occ));
  while(m)
  { to = bitscanf(m);
    bitclear(m, to);
    if(!is_pinned_b((to+8), to))
      prom_store_b(ms, count, (to+8), to, PROM);
  }
  //cap/prom - right side:
  m = ((pcs & ~FMASK_H)>> 7) & pos->occ[OCC_W];
  while(m)
  { to = bitscanf(m);
    bitclear(m, to);
    if(!is_pinned_b((to+7), to))
      prom_store_b(ms, count, (to+7), to, CAP|PROM);
  }
  //cap/prom - left side:
  m = ((pcs & ~FMASK_A) >> 9) & pos->occ[OCC_W];
  while(m)
  { to = bitscanf(m);
    bitclear(m, to);
    if(!is_pinned_b((to+9), to))
      prom_store_b(ms, count, (to+9), to, CAP|PROM);
  }

  //pawn captures:
  pcs = (pos->occ[BP] & ~(0xFFULL << 8)); //pawns that are not on 2nd rank
  //right side:
  m = ((pcs & ~FMASK_H) >> 7) & pos->occ[OCC_W];
  while(m)
  { to = bitscanf(m);
    bitclear(m, to);
    if(!is_pinned_b((to+7), to))
      move_store(ms, count, (to+7), to, CAP, 0);
  }
  //left side:
  m = ((pcs & ~FMASK_A) >> 9) & pos->occ[OCC_W];
  while(m)
  { to = bitscanf(m);
    bitclear(m, to);
    if(!is_pinned_b((to+9), to))
      move_store(ms, count, (to+9), to, CAP, 0);
  }

  //e.p.:
  if(pos->ep && pos->square[pos->ep] == EMPTY
  && pos->square[pos->ep+8] == WP)
  { if(pos->square[pos->ep+7] == BP
    && (p_attacks[B][pos->ep+7] & (1ULL << pos->ep)))
    { if(!is_pinned_ep_b((pos->ep+7), pos->ep))
        move_store(ms, count, (pos->ep+7), pos->ep, (CAP|EP), 0);
    }
    if(pos->square[pos->ep+9] == BP
    && (p_attacks[B][pos->ep+9] & (1ULL << pos->ep)))
    { if(!is_pinned_ep_b((pos->ep+9), pos->ep))
        move_store(ms, count, (pos->ep+9), pos->ep, (CAP|EP), 0);
    }
  }

  ///knight moves 
  pcs = pos->occ[BN];
  while(pcs)
  { from = bitscanf(pcs);
    bitclear(pcs, from);
    //captures:
    m = n_moves[from] & pos->occ[OCC_W];
    while(m)
    { to = bitscanf(m);
      bitclear(m, to);
      if(!is_pinned_b(from, to))
        move_store(ms, count, from, to, CAP, 0);
    }
  }

  ///king moves:
  from = bitscanf(pos->occ[BK]);
  //captures:
  m = k_moves[from] & pos->occ[OCC_W];
  while(m)
  { to = bitscanf(m);
    bitclear(m, to);
    if(!is_sq_attacked_w(to))
      move_store(ms, count, from, to, CAP, 0);
  }

  ///queen moves:
  pcs = pos->occ[BQ];
  while(pcs)
  { from = bitscanf(pcs);
    bitclear(pcs, from);
    m = bmoves(from, (occ)) | rmoves(from, (occ));
    m &= ~pos->occ[OCC_B];
    c = m & pos->occ[OCC_W];
    m &= ~pos->occ[OCC_W];

    //captures:
    while(c)
    { to = bitscanf(c);
      bitclear(c, to);
      if(!is_pinned_b(from, to))
        move_store(ms, count, from, to, CAP, 0);
    }
  }

  ///rook moves:
  pcs = pos->occ[BR];
  while(pcs)
  { from = bitscanf(pcs);
    bitclear(pcs, from);
    m = rmoves(from, (occ));
    m &= ~pos->occ[OCC_B];
    c = m & pos->occ[OCC_W];
    m &= ~pos->occ[OCC_W];

    //captures:
    while(c)
    { to = bitscanf(c);
      bitclear(c, to);
      if(!is_pinned_b(from, to))
        move_store(ms, count, from, to, CAP, 0);
    }
  }

  ///bishop moves:
  pcs = pos->occ[BB];
  while(pcs)
  { from = bitscanf(pcs);
    bitclear(pcs, from);
    m = bmoves(from, (occ));
    m &= ~pos->occ[OCC_B];
    c = m & pos->occ[OCC_W];
    m &= ~pos->occ[OCC_W];

    //captures:
    while(c)
    { to = bitscanf(c);
      bitclear(c, to);
      if(!is_pinned_b(from, to))
        move_store(ms, count, from, to, CAP, 0);
    }
  }

  return count;
}

int move_gen_caps(move_t *ms)
{
  if(pos->side==W) return move_gen_caps_w(ms);
  else return move_gen_caps_b(ms);
}
