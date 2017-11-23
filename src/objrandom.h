/**************************************************************************
 * #   #   #   ##   #  #  ###   ##   ##  ###       http://www.lyonesse.it *
 * #    # #   #  #  ## #  #    #    #    #                                *
 * #     #    #  #  # ##  ##    #    #   ##   ## ##  #  #  ##             *
 * #     #    #  #  # ##  #      #    #  #    # # #  #  #  # #            *
 * ###   #     ##   #  #  ###  ##   ##   ###  #   #  ####  ##    Ver. 1.0 *
 *                                                                        *
 * -Based on CircleMud & Smaug-     Copyright (c) 2001-2002 by Mithrandir *
 *                                                                        *
 * ********************************************************************** */

/* === Defines ============================================================ */

#define MAX_QLTY	5

/* === Structures ========================================================= */

typedef struct	quality_data		QUALITY_DATA;
typedef struct	rand_armor_data		RAND_ARMOR_DATA;
typedef struct	rand_weapon_data	RAND_WEAPON_DATA;
typedef struct	rand_worn_data		RAND_WORN_DATA;

struct quality_data
{
  char				*descr;
  int				bonus;
};

struct rand_armor_data
{
  char				*name;
  sh_int			protect_mod;	
  sh_int			magic_mod;
  int				wear_loc;	
};

struct rand_weapon_data
{
  char				*name;
  sh_int			magic_mod;
  sh_int			damage_mod;
  int				damage_type;
};

struct rand_worn_data
{
  char				*name;
  sh_int			magic_mod;
  int				wear_loc;
  int				extra_flags;	
};

/* === Functions Prototypes =============================================== */

OBJ_DATA *load_rand_obj(int type, int level);
OBJ_DATA *create_rand_armor(int level, bool magic);
OBJ_DATA *create_rand_shield(int level, bool magic);
OBJ_DATA *create_rand_arms(int level, bool magic);
OBJ_DATA *create_rand_legs(int level, bool magic);
OBJ_DATA *create_rand_hands(int level, bool magic);
OBJ_DATA *create_rand_feets(int level, bool magic);
OBJ_DATA *create_rand_belt(int level, bool magic);
OBJ_DATA *create_rand_helm(int level, bool magic);
OBJ_DATA *create_rand_weapon(int level, bool magic);
OBJ_DATA *create_rand_worn(int level, bool magic);

