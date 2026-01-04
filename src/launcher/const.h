/*
** const.h
**
** Defines the consts to be used by the launcher
**
**---------------------------------------------------------------------------
**
** Copyright 2025-2026 UZDoom Maintainers and Contributors
**
** SPDX-License-Identifier: GPL-3.0-or-later
**
**---------------------------------------------------------------------------
**
*/

#pragma once

#include <string>
#include <string_view>
#include <wx/filename.h>
#include <wx/wx.h>
#include <wx/stdpaths.h>

// a precaution when shortcuts are used to get the actual launcher executable
inline wxString exePath;

// defined directories
inline wxString ROOT_DIR;
inline wxString PROFILE_DIR;

// config file path
inline wxString CONFIG_FILE;

// default language is u.s english (enu)
inline std::string_view DEFAULT_LANG = "enu";

// Simple struct to hold all data for a single flag
struct FlagInfo
{
	const char *label;
	const char *tooltip;
	int         setIdx; // 0 = dmflags or compatflags, 1 = dmflags2, compatflags2 etc.
	int         bitVal; // The actual value 1 << [flag index starting from 0]
	bool        invert; // true = Unchecking the box adds the flag (because some are default checked)
	wxCheckBox *ctrl;   // internal use
};


// store flags here
static std::vector<FlagInfo> dmFlags = {

	// DMFLAGS
	{					  "Allow Health (Deathmatch)","Allows players to pick up items which restore health",0,1 << 0,true																																									},
	{					"Allow Powerups (Deathmatch)",											  "Allows players to pick up and use powerup items", 0,   1 << 1,  true},
	{					  "Weapons Stay (Deathmatch)",
     "Weapons aren't removed from the map when a player picks them up; Doesn't work with weapons dropped by "
     "enemies", 0,   1 << 2, false																																		},
	{						   "Falling damage (Old)",                 "Damages the player when they fall too far; uses old ZDoom damage calculation", 0,   1 << 3,
     false																																								},
	{						 "Falling damage (Hexen)",                   "Damages the player when they fall too far; uses Hexen's damage calculation", 0,   1 << 4,
     false																																								},
	{						"Falling damage (Strife)",                  "Damages the player when they fall too far; uses Strife's damage calculation", 0,       24,
     false																																								}, //  it really is 24 according to ingame
	{						  "Same Map (Deathmatch)",             "After exiting, the current map is started over instead of proceeding to the next", 0,
     1 << 6, false																																						},
	{					"Spawn Farthest (Deathmatch)",                         "Tries to spawn players far away from each other in a Deathmatch game", 0,   1 << 7,
     false																																								},
	{								  "Force Respawn",									   "Forces the player to respawn a few seconds after death", 0,   1 << 8, false},
	{					   "Allow Armor (Deathmatch)",											 "Allows players to pick up items which give armor", 0,   1 << 9,  true},
	{						"Allow Exit (Deathmatch)",
     "If off, trying to exit the level by normal means will kill the offending player instead", 0,  1 << 10,  true                                                        },
	{								  "Infinite Ammo",															   "All players have infinite ammo", 0,  1 << 11, false},
	{									"No Monsters",
     "Monsters placed in the map don't appear; does not stop monsters from spawning via scripts, however", 0,  1 << 12,
     false																																								},
	{							   "Monsters Respawn",
     "Monsters who are killed automatically respawn; this is the default behavior for the Nightmare skill setting", 0,
     1 << 13, false																																					   },
	{								  "Items Respawn",						 "Basic items, such as weapons and ammo, respawn after being picked up", 0,  1 << 14, false},
	{								  "Fast Monsters", "Monsters move and react faster; this is the default behavior for the Nightmare skill setting", 0,
     1 << 15, false																																					   },
	{									 "Allow Jump",						   "Players are allowed to jump; can be overridden in the MAPINFO lump", 0,  1 << 16,  true},
	{								 "Allow Freelook",               "Players are allowed to look up and down; can be overridden in the MAPINFO lump", 0,  1 << 18,
     true																																								 },
	{									  "Allow FOV",				   "Players are allowed to change their FOV (Field of Vision) from the default", 0,  1 << 20,  true},
	{						   "Spawn Multi. Weapons",      "If off, weapons marked as Multiplayer Only in the map won't appear in Cooperative games",
     0,  1 << 21,  true																																				   },
	{								   "Allow Crouch",						 "Players are allowed to crouch; can be overridden in the MAPINFO lump", 0,  1 << 22,  true},
	{				   "Lose Inventory (Cooperative)",                     "Players lose everything in their inventory upon death (Cooperative only)", 0,
     1 << 24, false																																					   },
	{						"Keep Keys (Cooperative)",									  "If off, players lose keys upon death (Cooperative only)", 0,  1 << 25,  true},
	{					 "Keep Weapons (Cooperative)",
     "If off, players lose all weapons upon death and are given the standard pistol and 50 bullets on respawn "
     "(Cooperative only)", 0,  1 << 26,  true																															 },
	{					   "Keep Armor (Cooperative)",									 "If off, armor is reset to 0% on death (Cooperative only)", 0,  1 << 27,  true},
	{					"Keep Powerups (Cooperative)",										"If off, powerups are lost on death (Cooperative only)", 0,  1 << 28,  true},
	{						"Keep Ammo (Cooperative)",
     "If off, ammo is lost on death and the player starts with only 50 bullets (Cooperative only)", 0,  1 << 29,  true                                                    },
	{				   "Lose Half Ammo (Cooperative)",                      "If on, half of the player's ammo is removed on death (Cooperative only)", 0,
     1 << 30, false																																					   },

	// DMFLAGS2
	{									"Drop Weapon",										"Drops the player's currently selected weapon on death", 1,   1 << 1, false},
	{				  "No Team Changing (Deathmatch)",
     "If on, players cannot change teams in a teamplay match after the map has started.", 1,   1 << 4, false                                                              },
	{									"Double Ammo",															 "If on, ammo pickups are doubled.", 1,   1 << 6, false},
	{								   "Degeneration",									  "Players have their health slowly drained when over 100%", 1,   1 << 7, false},
	{							   "Allow BFG Aiming",
     "Players are allowed to aim BFG shots up and down; turn off to prevent the BFG from being fired into the "
     "ground to generate instant tracers", 1,   1 << 8,  true																											 },
	{				   "Barrels Respawn (Deathmatch)",														"Respawns barrels in a Deathmatch game", 1,   1 << 9, false},
	{				"Respawn Protection (Deathmatch)",                                "Players who are respawning are invulnerable for a few seconds", 1,  1 << 10,
     false																																								},
	{				 "Spawn Where Died (Cooperative)",
     "If on, players respawn at the spot they died, unless they died in an instant-death sector (Sector type 115). "
     "(Coop only)", 1,  1 << 12, false																																	},
	{				 "Keep Frags Gained (Deathmatch)",                               "If on, players keep their frag count from one map to the next.", 1,  1 << 13,
     false																																								},
	{									 "No Respawn",								 "If enabled, players will not be able to respawn after dying.", 1,  1 << 14, false},
	{				"Lose Frag on Death (Deathmatch)",													  "Players lose a frag each time they die.", 1,  1 << 15, false},
	{							 "Infinite Inventory",													"Inventory items aren't removed when used.", 1,  1 << 16, false},
	{							"No Monsters to Exit",								   "Players cannot exit the map until all monsters are killed.", 1,  1 << 17, false},
	{								  "Allow Automap",												 "Players are allowed to access their automap.", 1,  1 << 18,  true},
	{								 "Automap Allies",										"Players can see other friendly players on the automap", 1,  1 << 19,  true},
	{								   "Allow Spying",					   "Players can use the spynext command to see through their allies' eyes.", 1,  1 << 20,  true},
	{								 "Chasecam Cheat",									   "Players are allowed to use the third-person chase cam.", 1,  1 << 21, false},
	{							   "Disallow Suicide",											 "Players cannot die due to self-inflicted damage.", 1,  1 << 22, false},
	{								  "Allow Autoaim",													   "Player weapons cannot auto aim if off.", 1,  1 << 23,  true},
	{							  "Check Ammo Switch",																  "Check ammo on weapon switch", 1,  1 << 24,  true},
	{						 "IoS Death Kills Spawns",										   "Icon of Sin (a.k.a Romero) death kills its spawns.", 1,  1 << 25,  true},
	{							  "End Sector Kill %",												 "End sector counts for total kill percentage.", 1,  1 << 26,  true},
	{						   "Big Powerups Respawn",                  "Powerups with the Inventory.BIGPOWERUP flag, respawn after being picked up.", 1,  1 << 27,
     false																																								},
	{				   "Allow vertical bullet spread",												"Vertical bullet spread for weapons is allowed", 1,  1 << 30, false},

	// DMFLAGS3
	{			   "No player clipping (Cooperative)",                                        "Players can walk through and shoot through each other", 2,   1 << 0, false},
	{					   "Share keys (Cooperative)",                              "Keys and other core items will be given to all players in coop.", 2,   1 << 1, false},
	{					"Local pickups (Cooperative)",
     "Items are picked up client-side rather than fully taken by the client who picked it up.", 2,   1 << 2, false                                                        },
	{"No local pickups of dropped items (Cooperative)",                                                  "Drops from Actors aren't picked up locally.", 2,   1 << 3,
     false																																								},
	{	  "Don't spawn coop-only items (Cooperative)",                                                "Items that only appear in co-op are disabled.", 2,   1 << 4, false},
	{	 "Don't spawn coop-only things (Cooperative)",                                            "Any Actor that only appears in co-op is disabled.", 2,   1 << 5,
     false																																								},
	{			 "Remember last weapon (Cooperative)",
     "When respawning in co-op, keep the last used weapon out instead of switching to the best new one.", 2,   1 << 6,
     false																																								},
	{					 "Pistol start (Cooperative)",									  "Clears player inventory when exiting to the next level.", 2,   1 << 7, false}
};



static std::vector<FlagInfo> compatFlags = {

	// compatflags
	{		  "Find shortest textures like Doom",
     "If enabled, Doom includes the first texture (normally treated as null) when determining move distance for "
     "specials that act upon the shortest surrounding texture (e.g. Floor_RaiseByTexture).", 0,1 << 0, false																												 },
	{				"Use buggier stair building",
     "If enabled, Doom's buggier stair-building code is used for the line specials that build stairs. See also the "
     "stair specials articles.", 0,		  1 << 1, false																			  },
	{		 "Limit Pain Elementals' Lost Souls",
     "Enables Doom's default behavior where pain elementals are now allowed to spawn new lost souls if there are "
     "already more than twenty on the map. Some older WADs took advantage of this limitation to create traps or "
     "ambushes where several Pain Elementals threaten the player but at first are unable to attack until the "
     "player grabs a powerup or otherwise triggers an action which kills enough Lost Souls to allow them to begin "
     "spawning more.", 0,		  1 << 2, false																						},
	{		"Don't let others hear your pickups",
     "In Doom, other players in a multiplayer match were not able to hear each other pick up items or weapons (the "
     "pickup sounds only played for the local player). ZDoom changes this so that players can hear other players' "
     "pickups. Enable this option to restore the original behavior.", 0,		  1 << 3, false										 },
	{				"Actors are infinitely tall",
     "Doom did not allow one actor to pass over the top of another; in fact, all actors were considered to be "
     "infinitely tall for the purposes of collision-detection with each other. ZDoom and other advanced ports "
     "change this so that objects can realistically move over or under each other. Enable this option to revert to "
     "Doom's original behavior.", 0,		  1 << 4, false																			 },
	{		"Cripple sound for silent BFG trick",
     "If enabled, players will only be allowed to emit one sound at a time. This Doom behavior can be exploited in "
     "multiplayer matches to mask certain sound effects (most notably the BFG firing sound) from other players. "
     "Note that this compatibility option heavily cripples ZDoom's sound system to achieve this effect and so "
     "players need to be aware of potential side-effects if they opt to enable this behavior.", 0,		  1 << 5, false               },
	{					   "Enable wall running",
     "Doom's collision-detection and movement routines were very basic and contained several known bugs that could "
     "be exploited to allow things that weren't originally intended. One of these was the ability to move "
     "extremely fast along walls oriented at a certain angle on the map. ZDoom's movement code fixes most of these "
     "issues, so it is no longer possible to use the \"wall - running\" cheat. A few maps however may have been "
     "designed with it in mind and become impossible to play without it, so this option is available in those "
     "cases. However, this heavily cripples ZDoom's movement code and re-introduces a number of bugs and "
     "inaccuracies. It is recommended to only use this option if absolutely necessary to complete the map.", 0,		  1 << 6, false  },
	{			 "Spawn item drops on the floor",
     "When monsters are killed in ZDoom, any items they drop are \"tossed\" into the air and then drop to the "
     "ground before coming to rest. Enable this option to restore the original behavior and make dropped items "
     "appear already on the floor.", 0,		  1 << 7, false																		  },
	{		 "All special lines can block <use>",
     "Doom contained a limitation where any line with a special (even one that does not activate anything when "
     "used) would intercept the player's use action and would not allow any lines behind it to trigger. By default "
     "ZDoom allows all lines within the player's reach to be triggered at once. Enable this option to restore the "
     "original behavior.", 0,		  1 << 8, false																					},
	{			"Disable Boom door light effect",
     "Boom (and ZDoom) add the ability to specify tagged sectors whose light level changes as a matching-tagged "
     "door is opened and closed. However, some older maps with incorrectly-tagged doors may inadvertently trigger "
     "this effect. Enable this option to prevent the light change from occuring in these instances.", 0,		  1 << 9, false         },
	{		"Raven scrollers use original speed",
     "Heretic and Hexen floor scrollers had the odd effect of visibly moving the floor texture at a slower rate "
     "than the player was carried. ZDoom corrects this glitch. The original effect can be restored by enabling "
     "this compatibility option.", 0,		 1 << 10, false																			},
	{		"Use original sound target handling",
     "Doom and older versions of ZDoom (up to 2.0.63a) used a sector flag to determine when monsters in each "
     "sector had heard the player. Since the flag never got reset once activated, monsters spawned into the map at "
     "a later time could wake up immediately without having to actually see or hear the player. Newer versions of "
     "ZDoom use a more realistic method by which enemies spawned into the map begin dormant and must be woken up "
     "in the usual way. However, certain older maps may rely on the original behavior and so enemies which are "
     "supposed to wake up immediately may remain dormant. Enable this option to restore the original "
     "functionality.", 0,		 1 << 11, false																						},
	{		"DEH health settings like Doom2.exe",
     "Boom introduced a known bug that caused DeHackEd's max health value to affect stimpacks and medikits in "
     "addition to health bonuses. ZDoom retains that same bug to allow maps to define a new maximum health value "
     "for players to remain compatible. To restore the original (correct) Doom behavior, enable this option.", 0,		 1 << 12, false},
	{"Self-referencing sectors don't block shots",
     "Doom ignored lines which had both sides in the same sector when determining whether a hitscan attack will "
     "pass through. ZDoom uses a more accurate routine which takes these lines into account. Enable this option to "
     "restore Doom's less accurate method.", 0,		 1 << 13, false																  },
	{		  "Monsters get stuck over dropoffs",
     "Originally, monsters using Doom's AI could get stuck if they were pushed onto a ledge and would be unable to "
     "move. ZDoom adds code that specifically checks for such a situation and finds a valid direction for the "
     "monster to move away from the ledge. This option disables that new movement code and allows monsters to "
     "remain stuck.", 0,		 1 << 14, false																						 },
	{			   "Boom scrollers are additive",
     "Boom's texture scrolling specials were designed to stack with each other and with Doom's default scroll "
     "types, however ZDoom does not use this additive behavior. Enable this option to use Boom's method and allow "
     "them to stack with each other.", 0,		 1 << 15, false																		},
	{			"Monsters see invisible players",
     "Enemies in ZDoom will not normally wake up when they \"see\" a player who is using an invisibility powerup. "
     "Enable this option to restore Doom's original behavior where enemies would always wake up in these "
     "circumstances.", 0,		 1 << 16, false																						},
	{	  "Instant moving floors are not silent",
     "If a sector moves instantly from one height to another, ZDoom will normally prevent any associated movement "
     "sounds from playing. This option re-enables the original Doom behavior where only the stop sound would be "
     "played in this cases.", 0,		 1 << 17, false																				 },
	{		"Sector sounds use center as source",
     "Doom and older versions of ZDoom considered the center of a sector to be the original point of any sounds "
     "that sector makes. In certain cases, this could cause the sound position to be inaccurate or players to hear "
     "a directional sound while standing within the sector that is generating it. This has since been fixed to "
     "where players hear the sound coming from the point of the sector nearest to them, ensuring an equal level of "
     "sound throughout the sector. This option will restore the older, less accurate sound behavior.", 0,		 1 << 18, false        },
	{	 "Use Doom heights for missile clipping",
     "If enabled, actors use their original heights for the purposes of projectile collision. This allows for "
     "decorations to be pass-through for projectiles as they were originally in Doom while still blocking other "
     "actors correctly. Specifically, this affects actors with negative values defined for their "
     "ProjectilePassHeight property.", 0,		 1 << 19, false																		},
	{			"Monsters cannot cross dropoffs",
     "Doom's physics code prevented enemies from being pushed off of ledges that are greater than the monster's "
     "maxstepheight property. ZDoom normally allows monsters to be pushed over these dropoffs by outside force. If "
     "enabled, this CVAR restores the original Doom behavior.", 0,		 1 << 20, false											   },
	{	 "Allow any bossdeath for level special",
     "Early versions of Doom executed the level's special action whenever the last monster of a kind that called "
     "A_BossDeath died. This allowed to have several different bosses on the same map, and have the special action "
     "repeated as many times, a fact that was used notably by 'Doomsday of UAC' to free a Cyberdemon once all "
     "Barons of Hell were defeated, and to make a red skull key accessible once the Cyberdemon was slain. Id "
     "Software considered this behavior a bug and fixed it, breaking this level.", 0,		 1 << 21, false							},
	{		 "No Minotaur floor flames in water",
     "Heretic introduced two new elements to the Doom engine: floor clipping to simulate actors wading through "
     "shallow water (or other liquids), and floor-hugging projectiles. The combination of the two, however, was "
     "not properly tested. When a maulotaur had its feet clipped by terrain and used its floor-hugging attack, the "
     "missiles were created below the floor, and were instantly destroyed as a result. Enabling this option "
     "prevents minotaurs from successfully creating their floor flames if their feet are clipped. Other "
     "floor-hugging projectiles are not affected.", 0,		 1 << 22, false														   },
	{	 "Original A_Mushroom speed in DEH mods",
     "Doom originally calculated a missile's velocity on the X and Y axes based on its Speed property, and then "
     "added a Z velocity to reach the point aimed at. In other words, the horizontal velocity was the same "
     "regardless of the angle, meaning that the higher you aimed, the faster the projectile actually was overall. "
     "Since ZDoom allows, through freelook, to aim much higher or much lower than is normally possible in Doom, "
     "the effect at steep angles looked visibly bugged and the formula was changed to derive all three components "
     "of the actor's velocity from its angle and pitch. However, MBF introduced a codepointer, A_Mushroom, that "
     "aimed projectiles at very steep angles, and the new ZDoom formula caused the effects of A_Mushroom to be "
     "very different in ZDoom compared to MBF. Enable this option to let A_Mushroom use the old formula when "
     "called from a state that was modified by DeHackEd.", 0,		 1 << 23, false													},
	{   "Monster movement is affected by effects",
     "Boom introduced sector friction and pusher/puller effects, and MBF subjected monsters to them. ZDoom, by "
     "default, does not, as the AI is unaware of such effects and incapable of coping up with them. Use this "
     "option to enable the MBF behavior. This does not affect \"conveyor belt\" effects.", 0,		 1 << 24, false                    },
	{	   "Crushed monsters can be resurrected",
     "Doom originally changed the state of an actor's corpse to the \"crushed gibs\" state if they were ground by "
     "a closing door, raising elevator, crusher, or similar effect. This behavior later led to a bug with the skin "
     "code in ZDoom as when a player's corpse was crushed, only its sprite's letter was changed, not the full "
     "sprite name, meaning that a crushed player corpse looked like a standing player. To solve the problem, the "
     "fix at the time was to remove the corpse and spawn in its place a gibs actor, and this in turn led to the "
     "result that arch-viles or similar monsters could no longer raise the monsters whose corpses had been "
     "crushed. Enabling this option to restore the original Doom behavior of changing the actor's state instead of "
     "replacing the actor. Note that player corpses are not affected, and any actor with a custom Crush state will "
     "use it in all cases.", 0,		 1 << 25, false																				  },
	{		  "Friendly monsters aren't blocked",
     "Friendly monsters are still monsters, and therefore blocked by monster-blocking lines. This can severely "
     "limit their utility, as for example a friendly monster summoned at the start of 'MAP01: Entryway' in Doom II "
     "will be unable to climb the steps of the triangle stairway. To counter this, MBF allowed any friendly "
     "monster to pass through monster-blocking lines. Enable this option to do the same.", 0,		 1 << 26, false                    },
	{					 "Invert sprite sorting",
     "ZDoom normally does not display overlapping sprites in the same order they were in Doom. Certain mods use "
     "overlapping sprites to achieve certain types of special effects, combining two different decorations into "
     "seemingly a single one. However, some mods require the original Doom order to work as intended, and others "
     "require the inverted ZDoom order. This compatibility option, if enabled, restores the original Doom order "
     "for sprite sorting.", 0,		 1 << 27, false																				   },
	{		  "Use Doom code for hitscan checks",
     "ZDoom fixed a couple of bugs in the hitscan trace routines, which had the effect of making hitscan attacks "
     "more efficient overall as in the original code they would sometimes \"magically\" miss. The first is that it "
     "is a monster's cross-section, rather than its bounding box, that is used to check for impact; this makes "
     "attacks with a limited range (especially player melee attacks) unlikely to hit very wide monsters. The "
     "second is the blockmap bug: if an actor crosses block boundaries and its center is in a different block than "
     "the one in which the impact happens, then there is no collision at all, letting attacks pass through it "
     "harmlessly. If enabled, this option restores the original, flawed behavior.", 0,		 1 << 28, false						   },
	{		  "Find neighboring light like Doom",
     "Doom had a logical bug in its algorithm to search for the highest light level in neighboring sectors, which "
     "prevented it from looking past the first tagged sector's neighbors. This bug was fixed in Boom, and later "
     "ZDoom adopted the fix as well. If enabled, this option uses the Doom behavior rather than the corrected Boom "
     "one.", 0,		 1 << 29, false																								  },
	{			   "Draw polyobjects like Hexen", "Uses the old flawed polyobject system, for maps that relied on its glitches.", 0,
     1 << 30, false																													 },
	{    "Ignore Y offsets on masked midtextures",
     "This option emulates a vanilla renderer glitch by ignoring the Y locations of patches drawn on two-sided "
     "midtextures and instead always drawing them at the top of the texture.", 0, (int)2147483648, false								}, //  fix checked bug

	// compatflags2
	{			   "Cannot travel straight NSEW",
     "This option emulates the error in the original engine's sine table by offsetting player angle when spawning "
     "or teleporting by one fineangle (approximatively 0.044°), preventing the player from facing directly in a "
     "cardinal direction.", 1,		  1 << 0, false																				   },
	{		  "Use Doom's floor motion behavior",
     "Vanilla Doom allows floors to move up past their ceilings, and ceilings to move down past their floors. "
     "ZDoom adopted a Boom fix to prevents this from happening. This compatibility option allows to turn off this "
     "fix.", 1,		  1 << 1, false																								  },
	{		   "Sounds stop when actor vanishes",
     "If enabled, a playing sound gets cut off if its source no longer exists in the game world.", 1,          1 << 2, false            },
	{		"Use Doom's point-on-line algorithm",
     "Re-enables Doom's original, bugged behaviour for deciding exactly what side of a line a point that sits "
     "exactly on a line should be on.", 1,		  1 << 3, false																	   },
	{"Level exit can be triggered more than once",
     "Allows level exits to be triggered multiple times. This is required by (and automatically applied to) "
     "Daedalus: Alien Defense's \"Travel Tube\" maps to work around some faulty scripting.", 1,		  1 << 4, false                  }
};


//store wad hash infos here too

// helper to better sort the data
struct WadInfo
{
	std::string title;
	std::string author;
	std::string releaseDate;
	bool        isIWAD = false; //force if wrong detection
};

// list 99% of versions is enough
static const std::unordered_map<std::string, WadInfo> wadDatabase = {
	// DOOM Shareware
	{"90facab21eede7981be10790e3f82da2",{"Doom (Shareware 1.0)", "id Software", "10-12-1993", true}                                        },
	{"cea4989df52b65f4d481b706234a3dca",
     {"Doom (Shareware 1.1)", "id Software", "15-12-1993", true}                                                      }, //  there are two versions
	{"52cbc8882f445573ce421fa5453513c1",                   {"Doom (Shareware 1.1)", "id Software", "16-12-1993", true}},
	{"2a380f28e813fb0989cae5e4762ebb4c",                   {"Doom (Shareware 1.2)", "id Software", "04-02-1994", true}},
	{"30aa5beb9e5ebfbbe1e1765561c08f38",
     {"Doom (Shareware 1.2)", "id Software", "17-02-1994", true}                                                      }, //  there are two versions
	{"17aebd6b5f2ed8ce07aa526a32af8d99",                  {"Doom (Shareware 1.25)", "id Software", "21-04-1994", true}},
	{"a21ae40c388cb6f2c3cc1b95589ee693",                   {"Doom (Shareware 1.4)", "id Software", "28-06-1994", true}},
	{"e280233d533dcc28c1acd6ccdc7742d4",                   {"Doom (Shareware 1.5)", "id Software", "08-07-1994", true}},
	{"762fd6d4b960d4b759730f01387a50a1",                   {"Doom (Shareware 1.6)", "id Software", "03-08-1994", true}},
	{"c428ea394dc52835f2580d5bfd50d76f",                 {"Doom (Shareware 1.666)", "id Software", "30-08-1994", true}},
	{"5f4eb849b1af12887dec04a2a12e5e62",                   {"Doom (Shareware 1.8)", "id Software", "10-12-1994", true}},
	{"f0cefca49926d00903cf57551d901abe",                   {"Doom (Shareware 1.9)", "id Software", "01-02-1995", true}},

	// DOOM Registered
	{"740901119ba2953e3c7f3764eca6e128",                  {"Doom (Registered 0.2)", "id Software", "04-02-1993", true}},
	{"dae9b1eea1a8e090fdfa5707187f4a43",                  {"Doom (Registered 0.3)", "id Software", "28-02-1993", true}},
	{"b6afa12a8b22e2726a8ff5bd249223de",                  {"Doom (Registered 0.4)", "id Software", "03-04-1993", true}},
	{"9c877480b8ef33b7074f1f0c07ed6487",                  {"Doom (Registered 0.5)", "id Software", "23-05-1993", true}},
	{"049e32f18d9c9529630366cfc72726ea",                 {"Doom (Registered Beta)", "id Software", "04-10-1993", true}},

	{"981b03e6d1dc033301aa3095acc437ce",                  {"Doom (Registered 1.1)", "id Software", "16-12-1993", true}},
	{"792fd1fea023d61210857089a7c1e351",                  {"Doom (Registered 1.2)", "id Software", "17-02-1994", true}},
	{"54978d12de87f162b9bcc011676cb3c0",                {"Doom (Registered 1.666)", "id Software", "01-09-1994", true}},
	{"11e1cd216801ea2657723abc86ecb01f",                  {"Doom (Registered 1.8)", "id Software", "20-01-1995", true}},
	{"1cd63c5ddff1bf8ce844237f580e9cf3",                  {"Doom (Registered 1.9)", "id Software", "01-02-1995", true}},
	{"c4fe9fd920207691a9f493668e0a2083",                      {"The Ultimate Doom", "id Software", "25-05-1995", true}},
	{"fb35c4a5a9fd49ec29ab6e900572c524",                     {"Doom (BFG Edition)", "id Software", "16-10-2012", true}},
	{"8517c4e8f0eef90b82852667d345eb86",    {"The Ultimate Doom (Unity/Bethesda)", "id Software", "09-01-2020	", true}},

	// DOOM II: HELL ON EARTH
	{"d9153ced9fd5b898b36cc5844e35b520",                 {"Doom II (1.666 German)", "id Software", "29-08-1994", true}},
	{"30e3c2d0350b67bfbf47271970b74b2f",                        {"Doom II (1.666)", "id Software", "29-08-1994", true}},
	{"ea74a47a791fdef2e9f2ea8b8a9da13b",                          {"Doom II (1.7)", "id Software", "21-09-1994", true}},
	{"d7a07e5d3f4625074312bc299d7ed33f",                         {"Doom II (1.7a)", "id Software", "18-10-1994", true}},
	{"3cb02349b3df649c86290907eed64e7b",                   {"Doom II (1.8 French)", "id Software", "01-12-1994", true}},
	{"c236745bb01d89bbb866c8fed81b6f8c",                          {"Doom II (1.8)", "id Software", "20-01-1995", true}},
	{"25e1459ca71d321525f84628f45ca8cd",                          {"Doom II (1.9)", "id Software", "01-02-1995", true}},
	{"b96683d113c4f4e9a916e1c7d1d71ffd",                        {"Doom II (PC-98)", "id Software", "01-02-1995", true}},
	{"c3bea40570c23e511a7ed3ebcd9865f7",
     {"Doom II (BFG Edition)", "id Software", "16-10-2012", true}                                                     }, //  appreantly its incorretly marked as PWAD
	{"8ab6d0527a29efdc1ef200e5687b5cae",               {"Doom II (Unity/Bethesda)", "id Software", "09-01-2020", true}},

	// TNT
	{"4e158d9953c79ccf97bd0663244cc6b6",            {"Final Doom: TNT Evilution (1.9)", "TeamTNT", "10-06-1996", true}},
	{"1d39e405bf6ee3df69a8d2646c8d5c49",      {"Final Doom: TNT Evilution (Anthology)", "TeamTNT", "14-11-1996", true}},
	{"f5528f6fd55cf9629141d79eda169630", {"Final Doom: TNT Evilution (Unity/Bethesda)", "TeamTNT", "03-09-2020", true}},

	// Plutonia
	{"75c8cf89566741fa9d22447604053bd7",    {"Final Doom: Plutonia Experiment", "Casali Brothers", "10-06-1996", true}},
	{"3493be7e1e2588bc9c8b31eab2587a04",
     {"Final Doom: Plutonia Experiment (Anthology)", "Casali Brothers", "21-11-1996", true}                           },
	{"ae76c20366ff685d3bb9fab11b148b84",
     {"Final Doom: Plutonia Experiment (Unity/Bethesda)", "Casali Brothers", "03-09-2020", true}                      },

	// Heretic
	{"ae779722390ec32fa37b0d361f7d82f8",             {"Heretic (Shareware 1.2)", "Raven Software", "28-06-1995", true}},
	{"023b52175d2f260c3bdc5528df5d0a8c",             {"Heretic (Shareware 1.0)", "Raven Software", "24-12-1994", true}},
	{"fc7eab659f6ee522bb57acc1a946912f",            {"Heretic (Shareware Beta)", "Raven Software", "23-12-1994", true}},

	{"66d686b1ed6d35ff103f15dbd30e0341",                       {"Heretic (1.3)", "Raven Software", "22-03-1996", true}},
	{"1e4cb4ef075ad344dd63971637307e04",                       {"Heretic (1.2)", "Raven Software", "28-06-1995", true}},
	{"3117e399cdb4298eaa3941625f4b2923",                       {"Heretic (1.0)", "Raven Software", "27-12-1994", true}},

	// Hexen
	{"abb033caf81e26f12a2103e1fa25453f",         {"Hexen: Beyond Heretic (1.1)", "Raven Software", "14-03-1996", true}},
	{"b2543a03521365261d0a0f74d5dd90f0",         {"Hexen: Beyond Heretic (1.0)", "Raven Software", "13-10-1995", true}},
	{"c88a2bb3d783e2ad7b599a8e301e099e",        {"Hexen: Beyond Heretic (Beta)", "Raven Software", "27-09-1995", true}},
	{"876a5a44c7b68f04b3bb9bc7a5bd69d6",        {"Hexen: Beyond Heretic (Demo)", "Raven Software", "18-10-1995", true}},
	{"9178a32a496ff5befebfe6c47dac106c",   {"Hexen: Beyond Heretic (Demo Beta)", "Raven Software", "02-10-1995", true}},

	{"78d5898e99e220e4de64edaa0e479593",
     {"Hexen: Deathkings of the Dark Citadel (1.1)", "Raven Software", "09-05-1996",
     false}																										   }, // Expansion , unrunnable without hexen
	{"1077432e2690d390c256ac908b5f4efa",
     {"Hexen: Deathkings of the Dark Citadel (1.0)", "Raven Software", "22-03-1996",
     false}																										   }, // Expansion , unrunnable without hexen

	// Strife
	{"bb545b9c4eca0ff92c14d466b3294023",            {"Strife (Teaser 1.1)", "Rogue Entertainment", "14-03-1996", true}},
	{"de2c8dcad7cca206292294bdab524292",            {"Strife (Teaser 1.0)", "Rogue Entertainment", "22-02-1996", true}},
	{"2fed2031a5b03892106e0f117f17901f",            {"Strife (1.2 - 1.31)", "Rogue Entertainment", "23-05-1996", true}},
	{"8f2d3a6a289f5d2f2f9c1eec02b47299",                   {"Strife (1.1)", "Rogue Entertainment", "18-04-1996", true}},
	{"082234d6a3f7086424856478b5aa9e95",
     {"Strife (Voices)", "Rogue Entertainment", "18-04-1996", false}                                                  }, //  just voices, needs game

	// Chex Quest 3
	{"bce163d06521f9d15f9686786e64df13",                  {"Chex Quest 3 (1.4)", "Charles Jacobi", "24-06-2009", true}},
	{"148367e53ff7f4f814e54b5ac9ff0ab3",                  {"Chex Quest 3 (1.3)", "Charles Jacobi", "12-06-2009", true}},
	{"26a8998ecdaa983f8e6c363b4b95bf55",                  {"Chex Quest 3 (1.2)", "Charles Jacobi", "02-05-2009", true}},
	{"f85944f55fff094f2ffbd3ecef3fa255",                  {"Chex Quest 3 (1.1)", "Charles Jacobi", "22-04-2009", true}},
	{"cb001c34e424687191f299cc1dff4d68",                  {"Chex Quest 3 (1.0)", "Charles Jacobi", "12-11-2008", true}},
	{"59c985995db55cd2623c1893550d82b3",   {"Chex Quest 3 (1.0 unoffical PWAD)", "Charles Jacobi", "24-06-2009", true}},

	// Action Doom 2: Urban Brawl
	{"1914b280b0a4b517214523bc2270e758",
     {"Action Doom 2: Urban Brawl (1.0)", "Stephen Browning et al.", "??-??-????", true}                              },
	{"c106a4e0a96f299954b073d5f97240be",
     {"Action Doom 2: Urban Brawl (1.1)", "Stephen Browning et al.", "26-12-2013", true}                              },

	// Hacx (Standalone 1.2)
	{"402ca45bb90520bfef0dec6baac5889e",                 {"Hacx (1.0 verified)", "Banjo Software", "08-10-1997", true}},
	{"1511a7032ebc834a3884cf390d7f186e",               {"Hacx (1.0 unverified)", "Banjo Software", "09-10-1997", true}},
	{"b7fd2f43f3382cf012dc6b097a3cb182",                          {"Hacx (1.1)", "Banjo Software", "16-09-1997", true}},
	{"65ed74d522bdf6649c2831b13b9e02b4",                          {"Hacx (1.2)", "Banjo Software", "09-10-2010", true}},
	{"793f07ebadb3d7353ee5b6b6429d9afa",
     {"Hacx (2.0)", "Banjo Software et al.", "09-10-2010", true}                                                      }, //  from build r61

	// Harmony
	{"48ebb49b52f6a3020d174dbcc1b9aeaf",                {"Harmony (1.1)", "Thomas van der Velden", "17-02-2012", true}},
	{"fe2cce6713ddcf6c6d6f0e8154b0cb38",                {"Harmony (1.0)", "Thomas van der Velden", "10-12-2009", true}},

	// The Adventures of Square (Ep 1 & 2)
	{"f4578097c658ad3c813cd5901ec125e2",       {"The Adventures of Square (2.1)", "BigBrik Games", "22-06-2019", true}},

	// Delaweare (this hash is taken from the wad of the standalone, as thats the only one i could find)
	{"a185498bdf721b4c01dc87fa81d1580b",                           {"Delaweare", "Space Is Green", "30-06-2014", true}},

	// Rise of the Wool Ball
	{"9176043468e10eaa471ae556e8e55745",           {"Rise of the Wool Ball (1.3)", "MSPaintR0cks", "21-07-2017", true}},
	{"fb0226e3fed7a3c1e7ea4cd4da905950",           {"Rise of the Wool Ball (1.2)", "MSPaintR0cks", "14-06-2017", true}},

	// Freedoom (MUST BE UPDATE EVERY RELASE)
	{"b93be13d05148dd01614bc205a03648e",          {"Freedoom: Phase 1 (0.13)", "Freedoom Project", "30-01-2024", true}},
	{"cd666466759b5e5f63af93c5f0ffd0a1",          {"Freedoom: Phase 2 (0.13)", "Freedoom Project", "30-01-2024", true}},
	{"908dfd77a14cc490c4cea94b62d13449",                     {"FreeDM (0.13)", "Freedoom Project", "30-01-2024", true}},
};
