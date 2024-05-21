//===== Hercules Plugin ======================================
//= @autoattack command
//===== By: ==================================================
//= chddrr
//= ossi0110
//===== Current Version: =====================================
//= 1.1
//===== Compatible With: ===================================== 
//= Hercules 2016-10-01
//===== Description: =========================================
// Original code by ossi0110, rewrriten by chddrr with added features and compatibility with Hercules 2024
// This plugin is extended/fork to allow the player to attack a specific monster, teleport/wak, use skills, etc.
// It is also intended to be compatible with latest Hercules version 2024 and will also release for rAthena.
// For original plugin, see:
// or contact ossi0110 on rAthena board / Discord: https://board.herc.ws/profile/87-ossi0110/
// This plugin is released under the Hercules Public License 2.0.
// I do not take any credit for the original plugin, I just rewrote it to be compatible with Hercules 2024 with an additional features.
//==== Original Features ==============================================
// - Allows players to toggle auto attack on and off
// - Players will automatically attack the nearest monster
// - Players will automatically walk, if no monster is in range
//==== Additional Features ============================================
// - Players can specify a monster ID to attack
// - Players can specify if they want to teleport or walk to search for monsters
//==== How to Use: ==================================================
// - @autoattack - Toggles auto attack on and off
// - @autoattack <monster_id> - Attacks the specified monster ID
// - @autoattack <monster_id> <teleport|walk> - Attacks the specified monster ID and teleports or walks to search for monsters (WIP)
//==== Changelog =====================================================
// 1.0 - Initial release and rewrite of the original plugin
// 1.1 - Added support for teleporting and walking to search for monsters


#include "common/hercules.h"
#include "map/atcommand.h"
#include "map/map.h"
#include "map/pc.h"
#include "map/script.h"
#include "map/unit.h"
#include "common/nullpo.h"
#include "common/timer.h"
#include <stdio.h>
#include <stdlib.h> // for rand()
#include <string.h>
#include "common/showmsg.h"
#include "common/HPMDataCheck.h"
#define OPTION_AUTOATTACK 0x10000000

int global_monster_id = 0;
bool enable_teleport = false; // if true, the player will teleport to search for monsters. This will override enable_walk
bool enable_walk = true; // if true, the player will walk to search for monsters, if false, the player will stand still and attack the nearest monster
bool enable_skills = false; // WIP: if true, the player will use skills to attack the monster, if false, the player will use normal attack
char skill_identity[24] = ""; // WIP: the name of the skill to use

HPExport struct hplugin_info pinfo = {
	"autoattack",		// Plugin  name
	SERVER_TYPE_MAP,	// Which server types this plugin works with?
	"1.1",				// Plugin version
	HPM_VERSION,		// HPM Version (don't change, macro is automatically updated)
};

static int buildin_autoattack_sub(struct block_list* bl, va_list ap)
{
	int* target_id = va_arg(ap, int*);
	*target_id = global_monster_id != 0 ? global_monster_id : bl->id;
	return 1;
}

void autoattack_motion(struct map_session_data* sd)
{
	int i, target_id = 0;

	//TODO: Add support for skills
	//TODO: Identify the monster id/address not the monster id in the database
	for (i = 0; i <= 9; i++)
	{
		if (global_monster_id != 0) {
			target_id = global_monster_id;
			map->foreachinarea(buildin_autoattack_sub, sd->bl.m, sd->bl.x - i, sd->bl.y - i, sd->bl.x + i, sd->bl.y + i, BL_MOB, &target_id);
			ShowInfo("parse called: %d\n", global_monster_id);
			unit->attack(&sd->bl, global_monster_id, 1);
			break;
		}
		else {
			map->foreachinarea(buildin_autoattack_sub, sd->bl.m, sd->bl.x - i, sd->bl.y - i, sd->bl.x + i, sd->bl.y + i, BL_MOB, &target_id);
			ShowInfo("parse called: %d\n", target_id);
			if (target_id)
			{
				unit->attack(&sd->bl, target_id, 1);
				break;
			}
		}
		target_id = 0;
	}

	if (!target_id)
	{
		/* Update walk_toxy, walktoxy doesn't exist in unit_interface anymore */
		//if  (!unit->walk_toxy) // TODO: Check if this is the correct way to check if walk_toxy exists
		unit->walk_toxy(&sd->bl, sd->bl.x + (rand() % 2 == 0 ? -1 : 1) * (rand() % 10), sd->bl.y + (rand() % 2 == 0 ? -1 : 1) * (rand() % 10), 0);
	}
	return;
}

int autoattack_timer(int tid, int64 tick, int id, intptr_t data)
{
	struct map_session_data* sd = NULL;

	sd = map->id2sd(id);
	if (sd == NULL)
		return 0;
	if (sd->sc.option & OPTION_AUTOATTACK)
	{
		autoattack_motion(sd);
		timer->add(timer->gettick() + 2000, autoattack_timer, sd->bl.id, 0);
	}
	return 0;
}

ACMD(autoattack)
{
	int monster_id = 0;
	int is_teleport = 0;
	int is_walking = 0;
	int skill_id = 0;

	// Scan and parse the parameter(s) provided by the player
	if (message && *message) {
		sscanf(message, "%d %d %d %d", &monster_id, &is_teleport, &is_walking, &skill_id);
	}

	// TODO: Add support for skills and checker for skill use
	enable_teleport = is_teleport == 1 ? true : false;
	enable_walk = is_walking == 1 ? true : false;
	enable_skills = skill_id ? true : false;
	//skill_identity = skill_id ? skill_id : 2;

	ShowInfo("parameters: %s\n", message);

	// Update the global variable with the provided monster ID
	global_monster_id = monster_id;

	if (sd->sc.option & OPTION_AUTOATTACK) {
		sd->sc.option &= ~OPTION_AUTOATTACK;
		unit->stop_attack(&sd->bl);
		clif->message(fd, "Auto Attack OFF");
	}
	else {
		sd->sc.option |= OPTION_AUTOATTACK;
		timer->add(timer->gettick() + 200, autoattack_timer, sd->bl.id, 0);
		clif->message(fd, "Auto Attack ON");
	}
	clif->changeoption(&sd->bl);
	return true;
}

/* Server Startup */
HPExport void plugin_init(void)
{
	addAtcommand("autoattack", autoattack);
}
