#define _MODULE_SEEDS

#include <stdlib.h>
#include <string.h>
#include "seeds.h"
#include "../common/util.h"

#define SEED__PATTERN_POSITIONS_SEP ':'
#define SEED__POSITIONS_SEP '|'

bool
parse_spaced_seed(char const * seed_string, struct seed_type * seed) {
  int i;
  const char * positions_string = NULL;
  char seed_pattern_end_marker = '\0';
  seed->mask[0] = 0x0;

#ifdef ENABLE_SEED_POSITIONS
  bitmap_long_reset(seed->positions, MAX_SEED_POSITIONS_BITMAP_SIZE);
  /* Is the seed positioned?
  If positions are given, the position list is separated from the seed pattern by ':';
  in the list, positions are separated by '|'*/
  positions_string = strchr(seed_string, SEED__PATTERN_POSITIONS_SEP);
  seed_pattern_end_marker = positions_string ? SEED__PATTERN_POSITIONS_SEP : '\0';
#endif
  seed->span = positions_string ? positions_string - seed_string : strlen(seed_string);
  seed->weight = strlchrcnt(seed_string, '1', seed_pattern_end_marker);
  if (seed->span < 1
      || seed->span > MAX_SEED_SPAN
      || seed->weight < 1
      || (int)strlchrcnt(seed_string, '0', seed_pattern_end_marker) != seed->span - seed->weight)
   return false;

  for (i = 0; i < seed->span; i++) {
    bitmap_prepend(seed->mask, 1, (seed_string[i] == '1' ? 1 : 0));
  }

#ifdef ENABLE_SEED_POSITIONS
  if (positions_string) {
    while (positions_string && strlen(positions_string) > 1 && sscanf (positions_string + 1, "%d", &i) > 0) {
      bitmap_long_insert_clean(seed->positions, 1, MAX_SEED_POSITIONS_BITMAP_SIZE, i, 1);
      positions_string = strchr(positions_string + 1, SEED__POSITIONS_SEP);
    }
  } else {
	 bitmap_long_allset(seed->positions, MAX_SEED_POSITIONS_BITMAP_SIZE);
  }
#endif
  return true;
}

bool
add_spaced_seed(char const * seed_string)
{
  int i;
  seed = (struct seed_type *)
    //xrealloc(seed, sizeof(struct seed_type) * (n_seeds + 1));
    my_realloc(seed, (n_seeds + 1) * sizeof(seed[0]), n_seeds * sizeof(seed[0]),
	       &mem_small, "seed");

  if (!parse_spaced_seed(seed_string, &seed[n_seeds])) {
    return false;
  }

  max_seed_span = MAX(max_seed_span, seed[n_seeds].span);
  min_seed_span = MIN(min_seed_span, seed[n_seeds].span);

  n_seeds++;

  avg_seed_span = 0;
  for (i = 0; i < n_seeds; i++)
    avg_seed_span += seed[i].span;
  avg_seed_span = avg_seed_span/n_seeds;

  return true;
}


void
load_default_mirna_seeds() {
  int i;
  for (i = 0; i < default_seeds_mirna_cnt; i++)
    add_spaced_seed(default_seeds_mirna[i]);
}


bool
load_default_seeds(int weight) {
  int i;

  //n_seeds = 0;
  switch(shrimp_mode) {
  case MODE_COLOUR_SPACE:
    if (weight == 0)
      weight = default_seeds_cs_weight;
    else if (weight < default_seeds_cs_min_weight || weight > default_seeds_cs_max_weight)
      return false;
    for (i = 0; i < default_seeds_cs_cnt[weight - default_seeds_cs_min_weight]; i++)
      add_spaced_seed(default_seeds_cs[weight - default_seeds_cs_min_weight][i]);
    break;
  case MODE_LETTER_SPACE:
    if (weight == 0)
      weight = default_seeds_ls_weight;
    else if (weight < default_seeds_ls_min_weight || weight > default_seeds_ls_max_weight)
      return false;
    for (i = 0; i < default_seeds_ls_cnt[weight - default_seeds_ls_min_weight]; i++)
      add_spaced_seed(default_seeds_ls[weight - default_seeds_ls_min_weight][i]);
    break;
  case MODE_HELICOS_SPACE:
    assert(0);
    break;
  }
  return true;
}


void
init_seed_hash_mask()
{
  int i, sn;

  seed_hash_mask = (uint32_t **)
    //xmalloc(sizeof(seed_hash_mask[0]) * n_seeds);
    my_malloc(n_seeds * sizeof(seed_hash_mask[0]),
	      &mem_small, "seed_hash_mask");
  for (sn = 0; sn < n_seeds; sn++) {
    seed_hash_mask[sn] = (uint32_t *)
      //xcalloc(sizeof(seed_hash_mask[sn][0]) * BPTO32BW(max_seed_span));
      my_calloc(BPTO32BW(max_seed_span) * sizeof(seed_hash_mask[sn][0]),
		&mem_small, "seed_hash_mask[%d]", sn);

    for (i = seed[sn].span - 1; i >= 0; i--)
      bitfield_prepend(seed_hash_mask[sn], max_seed_span,
		       bitmap_extract(seed[sn].mask, 1, i) == 1? 0xf : 0x0);
  }
}


char *
seed_to_string(int sn)
{
  static char buffer[100];
  bitmap_type tmp;
  int i;

  buffer[seed[sn].span] = 0;
  for (i = seed[sn].span - 1, tmp = seed[sn].mask[0];
       i >= 0;
       i--, tmp >>= 1) {
    if (bitmap_extract(&tmp, 1, 0) == 1)
      buffer[i] = '1';
    else
      buffer[i] = '0';
  }

  return buffer;
}


bool
valid_spaced_seeds()
{
  int sn;

  for (sn = 0; sn < n_seeds; sn++) {
    if (!Hflag && seed[sn].weight > MAX_SEED_WEIGHT)
      return false;

    if (Hflag && (seed[sn].span > MAX_HASH_SEED_SPAN ||
		  seed[sn].weight > MAX_HASH_SEED_WEIGHT))
      return false;
  }

  return true;
}
