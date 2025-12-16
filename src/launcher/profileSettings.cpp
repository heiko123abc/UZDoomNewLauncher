#include "ProfileSettings.h"
#include <filesystem>
#include <fstream>
#include <memory>
#include <vector>
#include <wx/combobox.h>
#include <wx/filepicker.h>
#include <wx/gbsizer.h>
#include <wx/listctrl.h>
#include <wx/notebook.h>
#include <wx/spinctrl.h>
#include <wx/wx.h>

#include <wx/utils.h>
#include <wx/valgen.h>
#include <wx/valnum.h>
#include <wx/valtext.h>

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

void ShowFlagEditor(Profile *currEdit, wxWindow *parent, const wxString &title, std::vector<FlagInfo> &flags,
                    int *definedVars, int varCount, bool showForceCheck = false)
{
	wxDialog dlg(parent, wxID_ANY, title, wxDefaultPosition, wxSize(500, 600));
	dlg.SetExtraStyle(dlg.GetExtraStyle() | wxWS_EX_VALIDATE_RECURSIVELY);
	dlg.SetWindowStyle(wxDEFAULT_DIALOG_STYLE & ~(wxRESIZE_BORDER | wxMAXIMIZE_BOX));

	wxBoxSizer       *mainSizer = new wxBoxSizer(wxVERTICAL);
	wxScrolledWindow *scrollWin =
		new wxScrolledWindow(&dlg, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL | wxBORDER_SUNKEN);
	scrollWin->SetScrollRate(0, 10);
	wxBoxSizer *scrollSizer = new wxBoxSizer(wxVERTICAL);

	// Create Checkboxes
	for (auto &f : flags)
	{
		f.ctrl = new wxCheckBox(scrollWin, wxID_ANY, f.label);
		f.ctrl->SetToolTip(f.tooltip);
		scrollSizer->Add(f.ctrl, 0, wxALL, 5);
	}
	scrollWin->SetSizer(scrollSizer);
	mainSizer->Add(scrollWin, 1, wxEXPAND | wxALL, 10);

	// Create Text Boxes
	std::vector<wxTextCtrl *> textCtrls;
	wxFlexGridSizer          *grid = new wxFlexGridSizer(2, 5, 10);

	if (showForceCheck)
	{
		// Checkbox for DMFlags
		wxCheckBox *forceBox = new wxCheckBox(&dlg, wxID_ANY, "Force Deathmatch-only flags for SP/Coop");
		forceBox->SetValidator(wxGenericValidator(&currEdit->alwaysapplydmflags));
		mainSizer->Add(forceBox, 0, wxALIGN_CENTER | wxBOTTOM, 5);
		dlg.TransferDataToWindow();
	}
	else
	{
		// Warning text for Compat flags
		mainSizer->Add(new wxStaticText(&dlg, wxID_ANY, "[!] Activating any Flags will disable the preset. [!]"), 0,
		               wxALL | wxALIGN_CENTER, 5);
	}

	for (int i = 0; i < varCount; i++)
	{
		// conditional textbox rendering
		if (title == "Additional Gameplay Options")
			grid->Add(new wxStaticText(&dlg, wxID_ANY, wxString::Format("dmflags%d:", i + 1)), 0,
			          wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
		if (title == "Custom Compatibility Options")
			grid->Add(new wxStaticText(&dlg, wxID_ANY, wxString::Format("compatflags%d:", i + 1)), 0,
			          wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);
		wxTextCtrl *tc = new wxTextCtrl(&dlg, wxID_ANY, "", wxDefaultPosition, wxSize(100, -1));
		textCtrls.push_back(tc);
		grid->Add(tc, 0, wxEXPAND);
	}

	mainSizer->Add(grid, 0, wxALIGN_CENTER | wxBOTTOM, 20);
	mainSizer->Add(new wxButton(&dlg, wxID_OK, "Return"), 0, wxALIGN_CENTER | wxBOTTOM, 15);

	auto UpdateUI = [&]() {
		std::vector<int> currentVals(varCount, 0);
		for (const auto &f : flags)
		{
			if (f.setIdx < varCount && (f.invert ? !f.ctrl->GetValue() : f.ctrl->GetValue()))
			{
				currentVals[f.setIdx] += f.bitVal;
			}
		}
		for (int i = 0; i < varCount; i++)
			textCtrls[i]->ChangeValue(wxString::Format("%d", currentVals[i])); // Use ChangeValue to avoid loop
	};

	auto UpdateBoxes = [&]() {
		std::vector<int> vals;
		for (auto *tc : textCtrls)
		{
			long v = 0;
			tc->GetValue().ToLong(&v);
			vals.push_back((int)v);
		}
		for (const auto &f : flags)
		{
			if (f.setIdx < vals.size())
			{
				bool bitSet = (vals[f.setIdx] & f.bitVal) == f.bitVal;
				f.ctrl->SetValue(f.invert ? !bitSet : bitSet);
			}
		}
	};

	// Bindings
	for (auto &f : flags)
		f.ctrl->Bind(wxEVT_CHECKBOX, [&](wxCommandEvent &) { UpdateUI(); });
	for (auto *tc : textCtrls)
		tc->Bind(wxEVT_TEXT, [&](wxCommandEvent &) { UpdateBoxes(); });

	// Initial Load
	for (int i = 0; i < varCount; i++)
		textCtrls[i]->ChangeValue(wxString::Format("%d", definedVars[i]));
	UpdateBoxes();

	dlg.SetSizer(mainSizer);
	dlg.Layout();

	if (dlg.ShowModal() == wxID_OK)
	{
		dlg.TransferDataFromWindow(); // Save the bool validator if it exists
		for (int i = 0; i < varCount; i++)
		{
			long v = 0;
			textCtrls[i]->GetValue().ToLong(&v);
			definedVars[i] = (int)v;
		}
	}
}

void advGameplay(Profile *currEdit, wxWindow *parent)
{

	static std::vector<FlagInfo> flags = {

		// DMFLAGS
		{					  "Allow Health (Deathmatch)","Allows players to pick up items which restore health", 0,1 << 0,true																																						   },
		{					"Allow Powerups (Deathmatch)",                                  "Allows players to pick up and use powerup items", 0,  1 << 1,  true},
		{					  "Weapons Stay (Deathmatch)",
	     "Weapons aren't removed from the map when a player picks them up; Doesn't work with weapons dropped by "
	     "enemies", 0,  1 << 2, false																															},
		{						   "Falling damage (Old)",     "Damages the player when they fall too far; uses old ZDoom damage calculation", 0,
	     1 << 3, false																																		   },
		{						 "Falling damage (Hexen)",       "Damages the player when they fall too far; uses Hexen's damage calculation", 0,
	     1 << 4, false																																		   },
		{						"Falling damage (Strife)",      "Damages the player when they fall too far; uses Strife's damage calculation", 0,
	     24, false																																			   }, //  it really is 24 according to ingame
		{						  "Same Map (Deathmatch)", "After exiting, the current map is started over instead of proceeding to the next", 0,
	     1 << 6, false																																		   },
		{					"Spawn Farthest (Deathmatch)",             "Tries to spawn players far away from each other in a Deathmatch game", 0,
	     1 << 7, false																																		   },
		{								  "Force Respawn",						   "Forces the player to respawn a few seconds after death", 0,  1 << 8, false},
		{					   "Allow Armor (Deathmatch)",                                 "Allows players to pick up items which give armor", 0,  1 << 9,  true},
		{						"Allow Exit (Deathmatch)",
	     "If off, trying to exit the level by normal means will kill the offending player instead", 0, 1 << 10,  true                                            },
		{								  "Infinite Ammo",												   "All players have infinite ammo", 0, 1 << 11, false},
		{									"No Monsters",
	     "Monsters placed in the map don't appear; does not stop monsters from spawning via scripts, however", 0,
	     1 << 12, false																																		  },
		{							   "Monsters Respawn",
	     "Monsters who are killed automatically respawn; this is the default behavior for the Nightmare skill setting", 0, 1 << 13, false                        },
		{								  "Items Respawn",             "Basic items, such as weapons and ammo, respawn after being picked up", 0, 1 << 14, false},
		{								  "Fast Monsters",
	     "Monsters move and react faster; this is the default behavior for the Nightmare skill setting", 0, 1 << 15,
	     false																																				   },
		{									 "Allow Jump",               "Players are allowed to jump; can be overridden in the MAPINFO lump", 0, 1 << 16,  true},
		{								 "Allow Freelook",   "Players are allowed to look up and down; can be overridden in the MAPINFO lump", 0, 1 << 18,
	     true																																					},
		{									  "Allow FOV",       "Players are allowed to change their FOV (Field of Vision) from the default", 0, 1 << 20,  true},
		{						   "Spawn Multi. Weapons",
	     "If off, weapons marked as Multiplayer Only in the map won't appear in Cooperative games", 0, 1 << 21,  true                                            },
		{								   "Allow Crouch",             "Players are allowed to crouch; can be overridden in the MAPINFO lump", 0, 1 << 22,  true},
		{				   "Lose Inventory (Cooperative)",         "Players lose everything in their inventory upon death (Cooperative only)", 0,
	     1 << 24, false																																		  },
		{						"Keep Keys (Cooperative)",                          "If off, players lose keys upon death (Cooperative only)", 0, 1 << 25,  true},
		{					 "Keep Weapons (Cooperative)",
	     "If off, players lose all weapons upon death and are given the standard pistol and 50 bullets on respawn "
	     "(Cooperative only)", 0, 1 << 26,  true																												 },
		{					   "Keep Armor (Cooperative)",                         "If off, armor is reset to 0% on death (Cooperative only)", 0, 1 << 27,  true},
		{					"Keep Powerups (Cooperative)",                            "If off, powerups are lost on death (Cooperative only)", 0, 1 << 28,  true},
		{						"Keep Ammo (Cooperative)",
	     "If off, ammo is lost on death and the player starts with only 50 bullets (Cooperative only)", 0, 1 << 29,
	     true																																					},
		{				   "Lose Half Ammo (Cooperative)",          "If on, half of the player's ammo is removed on death (Cooperative only)", 0,
	     1 << 30, false																																		  },

		// DMFLAGS2
		{									"Drop Weapon",							"Drops the player's currently selected weapon on death", 1,  1 << 1, false},
		{				  "No Team Changing (Deathmatch)",
	     "If on, players cannot change teams in a teamplay match after the map has started.", 1,  1 << 4, false                                                  },
		{									"Double Ammo",												 "If on, ammo pickups are doubled.", 1,  1 << 6, false},
		{								   "Degeneration",						  "Players have their health slowly drained when over 100%", 1,  1 << 7, false},
		{							   "Allow BFG Aiming",
	     "Players are allowed to aim BFG shots up and down; turn off to prevent the BFG from being fired into the "
	     "ground to generate instant tracers", 1,  1 << 8,  true																								 },
		{				   "Barrels Respawn (Deathmatch)",											"Respawns barrels in a Deathmatch game", 1,  1 << 9, false},
		{				"Respawn Protection (Deathmatch)",                    "Players who are respawning are invulnerable for a few seconds", 1, 1 << 10,
	     false																																				   },
		{				 "Spawn Where Died (Cooperative)",
	     "If on, players respawn at the spot they died, unless they died in an instant-death sector (Sector type 115). "
	     "(Coop only)", 1, 1 << 12, false																														},
		{				 "Keep Frags Gained (Deathmatch)",                   "If on, players keep their frag count from one map to the next.", 1, 1 << 13,
	     false																																				   },
		{									 "No Respawn",                     "If enabled, players will not be able to respawn after dying.", 1, 1 << 14, false},
		{				"Lose Frag on Death (Deathmatch)",                                          "Players lose a frag each time they die.", 1, 1 << 15, false},
		{							 "Infinite Inventory",										"Inventory items aren't removed when used.", 1, 1 << 16, false},
		{							"No Monsters to Exit",                       "Players cannot exit the map until all monsters are killed.", 1, 1 << 17, false},
		{								  "Allow Automap",									 "Players are allowed to access their automap.", 1, 1 << 18,  true},
		{								 "Automap Allies",							"Players can see other friendly players on the automap", 1, 1 << 19,  true},
		{								   "Allow Spying",           "Players can use the spynext command to see through their allies' eyes.", 1, 1 << 20,  true},
		{								 "Chasecam Cheat",						   "Players are allowed to use the third-person chase cam.", 1, 1 << 21, false},
		{							   "Disallow Suicide",								 "Players cannot die due to self-inflicted damage.", 1, 1 << 22, false},
		{								  "Allow Autoaim",										   "Player weapons cannot auto aim if off.", 1, 1 << 23,  true},
		{							  "Check Ammo Switch",													  "Check ammo on weapon switch", 1, 1 << 24,  true},
		{						 "IoS Death Kills Spawns",                               "Icon of Sin (a.k.a Romero) death kills its spawns.", 1, 1 << 25,  true},
		{							  "End Sector Kill %",									 "End sector counts for total kill percentage.", 1, 1 << 26,  true},
		{						   "Big Powerups Respawn",      "Powerups with the Inventory.BIGPOWERUP flag, respawn after being picked up.", 1,
	     1 << 27, false																																		  },
		{				   "Allow vertical bullet spread",                                    "Vertical bullet spread for weapons is allowed", 1, 1 << 30, false},

		// DMFLAGS3
		{			   "No player clipping (Cooperative)",                            "Players can walk through and shoot through each other", 2,  1 << 0, false},
		{					   "Share keys (Cooperative)",                  "Keys and other core items will be given to all players in coop.", 2,  1 << 1,
	     false																																				   },
		{					"Local pickups (Cooperative)",
	     "Items are picked up client-side rather than fully taken by the client who picked it up.", 2,  1 << 2, false                                            },
		{"No local pickups of dropped items (Cooperative)",                                      "Drops from Actors aren't picked up locally.", 2,  1 << 3,
	     false																																				   },
		{      "Don't spawn coop-only items (Cooperative)",                                    "Items that only appear in co-op are disabled.", 2,  1 << 4,
	     false																																				   },
		{     "Don't spawn coop-only things (Cooperative)",                                "Any Actor that only appears in co-op is disabled.", 2,  1 << 5,
	     false																																				   },
		{			 "Remember last weapon (Cooperative)",
	     "When respawning in co-op, keep the last used weapon out instead of switching to the best new one.", 2,  1 << 6,
	     false																																				   },
		{					 "Pistol start (Cooperative)",                          "Clears player inventory when exiting to the next level.", 2,  1 << 7, false}
    };

	// Pass the 3 variables by reference in an array
	int vars[] = {currEdit->DMFlags, currEdit->DMFlags2, currEdit->DMFlags3};

	ShowFlagEditor(currEdit, parent, "Additional Gameplay Options", flags, vars, 3, true);

	// Save back results
	currEdit->DMFlags  = vars[0];
	currEdit->DMFlags2 = vars[1];
	currEdit->DMFlags3 = vars[2];
}

void advCompat(Profile *currEdit, wxWindow *parent)
{

	static std::vector<FlagInfo> flags = {

		// compatflags
		{		  "Find shortest textures like Doom",
	     "If enabled, Doom includes the first texture (normally treated as null) when determining move distance for "
	     "specials that act upon the shortest surrounding texture (e.g. Floor_RaiseByTexture).",0,          1 << 0,false																																	  },
		{				"Use buggier stair building",
	     "If enabled, Doom's buggier stair-building code is used for the line specials that build stairs. See also the "
	     "stair specials articles.", 0,          1 << 1, false																			  },
		{		 "Limit Pain Elementals' Lost Souls",
	     "Enables Doom's default behavior where pain elementals are now allowed to spawn new lost souls if there are "
	     "already more than twenty on the map. Some older WADs took advantage of this limitation to create traps or "
	     "ambushes where several Pain Elementals threaten the player but at first are unable to attack until the "
	     "player grabs a powerup or otherwise triggers an action which kills enough Lost Souls to allow them to begin "
	     "spawning more.", 0,          1 << 2, false																						},
		{        "Don't let others hear your pickups",
	     "In Doom, other players in a multiplayer match were not able to hear each other pick up items or weapons (the "
	     "pickup sounds only played for the local player). ZDoom changes this so that players can hear other players' "
	     "pickups. Enable this option to restore the original behavior.", 0,          1 << 3, false										 },
		{				"Actors are infinitely tall",
	     "Doom did not allow one actor to pass over the top of another; in fact, all actors were considered to be "
	     "infinitely tall for the purposes of collision-detection with each other. ZDoom and other advanced ports "
	     "change this so that objects can realistically move over or under each other. Enable this option to revert to "
	     "Doom's original behavior.", 0,          1 << 4, false																			 },
		{        "Cripple sound for silent BFG trick",
	     "If enabled, players will only be allowed to emit one sound at a time. This Doom behavior can be exploited in "
	     "multiplayer matches to mask certain sound effects (most notably the BFG firing sound) from other players. "
	     "Note that this compatibility option heavily cripples ZDoom's sound system to achieve this effect and so "
	     "players need to be aware of potential side-effects if they opt to enable this behavior.", 0,          1 << 5, false               },
		{					   "Enable wall running",
	     "Doom's collision-detection and movement routines were very basic and contained several known bugs that could "
	     "be exploited to allow things that weren't originally intended. One of these was the ability to move "
	     "extremely fast along walls oriented at a certain angle on the map. ZDoom's movement code fixes most of these "
	     "issues, so it is no longer possible to use the \"wall - running\" cheat. A few maps however may have been "
	     "designed with it in mind and become impossible to play without it, so this option is available in those "
	     "cases. However, this heavily cripples ZDoom's movement code and re-introduces a number of bugs and "
	     "inaccuracies. It is recommended to only use this option if absolutely necessary to complete the map.", 0,          1 << 6, false  },
		{			 "Spawn item drops on the floor",
	     "When monsters are killed in ZDoom, any items they drop are \"tossed\" into the air and then drop to the "
	     "ground before coming to rest. Enable this option to restore the original behavior and make dropped items "
	     "appear already on the floor.", 0,          1 << 7, false																		  },
		{		 "All special lines can block <use>",
	     "Doom contained a limitation where any line with a special (even one that does not activate anything when "
	     "used) would intercept the player's use action and would not allow any lines behind it to trigger. By default "
	     "ZDoom allows all lines within the player's reach to be triggered at once. Enable this option to restore the "
	     "original behavior.", 0,          1 << 8, false																					},
		{			"Disable Boom door light effect",
	     "Boom (and ZDoom) add the ability to specify tagged sectors whose light level changes as a matching-tagged "
	     "door is opened and closed. However, some older maps with incorrectly-tagged doors may inadvertently trigger "
	     "this effect. Enable this option to prevent the light change from occuring in these instances.", 0,          1 << 9, false         },
		{        "Raven scrollers use original speed",
	     "Heretic and Hexen floor scrollers had the odd effect of visibly moving the floor texture at a slower rate "
	     "than the player was carried. ZDoom corrects this glitch. The original effect can be restored by enabling "
	     "this compatibility option.", 0,         1 << 10, false																			},
		{        "Use original sound target handling",
	     "Doom and older versions of ZDoom (up to 2.0.63a) used a sector flag to determine when monsters in each "
	     "sector had heard the player. Since the flag never got reset once activated, monsters spawned into the map at "
	     "a later time could wake up immediately without having to actually see or hear the player. Newer versions of "
	     "ZDoom use a more realistic method by which enemies spawned into the map begin dormant and must be woken up "
	     "in the usual way. However, certain older maps may rely on the original behavior and so enemies which are "
	     "supposed to wake up immediately may remain dormant. Enable this option to restore the original "
	     "functionality.", 0,         1 << 11, false																						},
		{        "DEH health settings like Doom2.exe",
	     "Boom introduced a known bug that caused DeHackEd's max health value to affect stimpacks and medikits in "
	     "addition to health bonuses. ZDoom retains that same bug to allow maps to define a new maximum health value "
	     "for players to remain compatible. To restore the original (correct) Doom behavior, enable this option.", 0,         1 << 12, false},
		{"Self-referencing sectors don't block shots",
	     "Doom ignored lines which had both sides in the same sector when determining whether a hitscan attack will "
	     "pass through. ZDoom uses a more accurate routine which takes these lines into account. Enable this option to "
	     "restore Doom's less accurate method.", 0,         1 << 13, false																  },
		{		  "Monsters get stuck over dropoffs",
	     "Originally, monsters using Doom's AI could get stuck if they were pushed onto a ledge and would be unable to "
	     "move. ZDoom adds code that specifically checks for such a situation and finds a valid direction for the "
	     "monster to move away from the ledge. This option disables that new movement code and allows monsters to "
	     "remain stuck.", 0,         1 << 14, false																						 },
		{			   "Boom scrollers are additive",
	     "Boom's texture scrolling specials were designed to stack with each other and with Doom's default scroll "
	     "types, however ZDoom does not use this additive behavior. Enable this option to use Boom's method and allow "
	     "them to stack with each other.", 0,         1 << 15, false																		},
		{			"Monsters see invisible players",
	     "Enemies in ZDoom will not normally wake up when they \"see\" a player who is using an invisibility powerup. "
	     "Enable this option to restore Doom's original behavior where enemies would always wake up in these "
	     "circumstances.", 0,         1 << 16, false																						},
		{      "Instant moving floors are not silent",
	     "If a sector moves instantly from one height to another, ZDoom will normally prevent any associated movement "
	     "sounds from playing. This option re-enables the original Doom behavior where only the stop sound would be "
	     "played in this cases.", 0,         1 << 17, false																				 },
		{        "Sector sounds use center as source",
	     "Doom and older versions of ZDoom considered the center of a sector to be the original point of any sounds "
	     "that sector makes. In certain cases, this could cause the sound position to be inaccurate or players to hear "
	     "a directional sound while standing within the sector that is generating it. This has since been fixed to "
	     "where players hear the sound coming from the point of the sector nearest to them, ensuring an equal level of "
	     "sound throughout the sector. This option will restore the older, less accurate sound behavior.", 0,         1 << 18, false        },
		{     "Use Doom heights for missile clipping",
	     "If enabled, actors use their original heights for the purposes of projectile collision. This allows for "
	     "decorations to be pass-through for projectiles as they were originally in Doom while still blocking other "
	     "actors correctly. Specifically, this affects actors with negative values defined for their "
	     "ProjectilePassHeight property.", 0,         1 << 19, false																		},
		{			"Monsters cannot cross dropoffs",
	     "Doom's physics code prevented enemies from being pushed off of ledges that are greater than the monster's "
	     "maxstepheight property. ZDoom normally allows monsters to be pushed over these dropoffs by outside force. If "
	     "enabled, this CVAR restores the original Doom behavior.", 0,         1 << 20, false											   },
		{     "Allow any bossdeath for level special",
	     "Early versions of Doom executed the level's special action whenever the last monster of a kind that called "
	     "A_BossDeath died. This allowed to have several different bosses on the same map, and have the special action "
	     "repeated as many times, a fact that was used notably by 'Doomsday of UAC' to free a Cyberdemon once all "
	     "Barons of Hell were defeated, and to make a red skull key accessible once the Cyberdemon was slain. Id "
	     "Software considered this behavior a bug and fixed it, breaking this level.", 0,         1 << 21, false							},
		{		 "No Minotaur floor flames in water",
	     "Heretic introduced two new elements to the Doom engine: floor clipping to simulate actors wading through "
	     "shallow water (or other liquids), and floor-hugging projectiles. The combination of the two, however, was "
	     "not properly tested. When a maulotaur had its feet clipped by terrain and used its floor-hugging attack, the "
	     "missiles were created below the floor, and were instantly destroyed as a result. Enabling this option "
	     "prevents minotaurs from successfully creating their floor flames if their feet are clipped. Other "
	     "floor-hugging projectiles are not affected.", 0,         1 << 22, false														   },
		{     "Original A_Mushroom speed in DEH mods",
	     "Doom originally calculated a missile's velocity on the X and Y axes based on its Speed property, and then "
	     "added a Z velocity to reach the point aimed at. In other words, the horizontal velocity was the same "
	     "regardless of the angle, meaning that the higher you aimed, the faster the projectile actually was overall. "
	     "Since ZDoom allows, through freelook, to aim much higher or much lower than is normally possible in Doom, "
	     "the effect at steep angles looked visibly bugged and the formula was changed to derive all three components "
	     "of the actor's velocity from its angle and pitch. However, MBF introduced a codepointer, A_Mushroom, that "
	     "aimed projectiles at very steep angles, and the new ZDoom formula caused the effects of A_Mushroom to be "
	     "very different in ZDoom compared to MBF. Enable this option to let A_Mushroom use the old formula when "
	     "called from a state that was modified by DeHackEd.", 0,         1 << 23, false													},
		{   "Monster movement is affected by effects",
	     "Boom introduced sector friction and pusher/puller effects, and MBF subjected monsters to them. ZDoom, by "
	     "default, does not, as the AI is unaware of such effects and incapable of coping up with them. Use this "
	     "option to enable the MBF behavior. This does not affect \"conveyor belt\" effects.", 0,         1 << 24, false                    },
		{       "Crushed monsters can be resurrected",
	     "Doom originally changed the state of an actor's corpse to the \"crushed gibs\" state if they were ground by "
	     "a closing door, raising elevator, crusher, or similar effect. This behavior later led to a bug with the skin "
	     "code in ZDoom as when a player's corpse was crushed, only its sprite's letter was changed, not the full "
	     "sprite name, meaning that a crushed player corpse looked like a standing player. To solve the problem, the "
	     "fix at the time was to remove the corpse and spawn in its place a gibs actor, and this in turn led to the "
	     "result that arch-viles or similar monsters could no longer raise the monsters whose corpses had been "
	     "crushed. Enabling this option to restore the original Doom behavior of changing the actor's state instead of "
	     "replacing the actor. Note that player corpses are not affected, and any actor with a custom Crush state will "
	     "use it in all cases.", 0,         1 << 25, false																				  },
		{		  "Friendly monsters aren't blocked",
	     "Friendly monsters are still monsters, and therefore blocked by monster-blocking lines. This can severely "
	     "limit their utility, as for example a friendly monster summoned at the start of 'MAP01: Entryway' in Doom II "
	     "will be unable to climb the steps of the triangle stairway. To counter this, MBF allowed any friendly "
	     "monster to pass through monster-blocking lines. Enable this option to do the same.", 0,         1 << 26, false                    },
		{					 "Invert sprite sorting",
	     "ZDoom normally does not display overlapping sprites in the same order they were in Doom. Certain mods use "
	     "overlapping sprites to achieve certain types of special effects, combining two different decorations into "
	     "seemingly a single one. However, some mods require the original Doom order to work as intended, and others "
	     "require the inverted ZDoom order. This compatibility option, if enabled, restores the original Doom order "
	     "for sprite sorting.", 0,         1 << 27, false																				   },
		{		  "Use Doom code for hitscan checks",
	     "ZDoom fixed a couple of bugs in the hitscan trace routines, which had the effect of making hitscan attacks "
	     "more efficient overall as in the original code they would sometimes \"magically\" miss. The first is that it "
	     "is a monster's cross-section, rather than its bounding box, that is used to check for impact; this makes "
	     "attacks with a limited range (especially player melee attacks) unlikely to hit very wide monsters. The "
	     "second is the blockmap bug: if an actor crosses block boundaries and its center is in a different block than "
	     "the one in which the impact happens, then there is no collision at all, letting attacks pass through it "
	     "harmlessly. If enabled, this option restores the original, flawed behavior.", 0,         1 << 28, false						   },
		{		  "Find neighboring light like Doom",
	     "Doom had a logical bug in its algorithm to search for the highest light level in neighboring sectors, which "
	     "prevented it from looking past the first tagged sector's neighbors. This bug was fixed in Boom, and later "
	     "ZDoom adopted the fix as well. If enabled, this option uses the Doom behavior rather than the corrected Boom "
	     "one.", 0,         1 << 29, false																								  },
		{			   "Draw polyobjects like Hexen", "Uses the old flawed polyobject system, for maps that relied on its glitches.",
	     0,         1 << 30, false																										  },
		{    "Ignore Y offsets on masked midtextures",
	     "This option emulates a vanilla renderer glitch by ignoring the Y locations of patches drawn on two-sided "
	     "midtextures and instead always drawing them at the top of the texture.", 0, (int)2147483648, false                                }, //  fix checked bug

		// compatflags2
		{			   "Cannot travel straight NSEW",
	     "This option emulates the error in the original engine's sine table by offsetting player angle when spawning "
	     "or teleporting by one fineangle (approximatively 0.044°), preventing the player from facing directly in a "
	     "cardinal direction.", 1,          1 << 0, false																				   },
		{		  "Use Doom's floor motion behavior",
	     "Vanilla Doom allows floors to move up past their ceilings, and ceilings to move down past their floors. "
	     "ZDoom adopted a Boom fix to prevents this from happening. This compatibility option allows to turn off this "
	     "fix.", 1,          1 << 1, false																								  },
		{		   "Sounds stop when actor vanishes",
	     "If enabled, a playing sound gets cut off if its source no longer exists in the game world.", 1,          1 << 2,
	     false																															  },
		{        "Use Doom's point-on-line algorithm",
	     "Re-enables Doom's original, bugged behaviour for deciding exactly what side of a line a point that sits "
	     "exactly on a line should be on.", 1,          1 << 3, false																	   },
		{"Level exit can be triggered more than once",
	     "Allows level exits to be triggered multiple times. This is required by (and automatically applied to) "
	     "Daedalus: Alien Defense's \"Travel Tube\" maps to work around some faulty scripting.", 1,          1 << 4, false                  }
    };

	int vars[] = {currEdit->compatflags, currEdit->compatflags2};

	ShowFlagEditor(currEdit, parent, "Custom Compatibility Options", flags, vars, 2, false);

	currEdit->compatflags  = vars[0];
	currEdit->compatflags2 = vars[1];
}

// a helper to avoid duplication
void OpenPathPicker(wxWindow *parent, wxTextCtrl *targetInput, const wxString &title, bool isFolder,
                    const wxString &filter = "All files (*.*)|*.*")
{
	if (isFolder)
	{
		// a folder path is needed
		wxDirDialog dirDialog(parent, title, wxGetCwd() + ROOT_DIR.ToUTF8(), wxDD_DEFAULT_STYLE | wxDD_DIR_MUST_EXIST);
		if (dirDialog.ShowModal() == wxID_OK)
		{
			targetInput->SetValue(dirDialog.GetPath());
		}
	}
	else
	{
		// a file path is needed
		wxFileDialog fileDialog(parent, title, wxGetCwd() + ROOT_DIR.ToUTF8(), targetInput->GetValue(), filter,
		                        wxFD_OPEN | wxFD_FILE_MUST_EXIST);
		if (fileDialog.ShowModal() == wxID_OK)
		{
			targetInput->SetValue(fileDialog.GetPath());
		}
	}
}

void RefreshModList(Profile *currEdit, wxScrolledWindow *listWindow, wxBoxSizer *listSizer)
{

	listSizer->Clear(true); // force a redraw

	// Rebuild the list from the vector
	for (size_t i = 0; i < currEdit->modFiles.size(); ++i)
	{
		wxString item = currEdit->modFiles[i];

		// Create the row panel
		wxPanel *row = new wxPanel(listWindow);
		row->SetBackgroundColour(wxColour(220, 240, 255)); // Light blue for visibility
		wxBoxSizer *rowSizer = new wxBoxSizer(wxHORIZONTAL);

		// Mod Name (Just show file name, the path is way maybe too long)
		std::string   fname = std::filesystem::path(item.ToStdString()).filename().string();
		wxStaticText *label = new wxStaticText(row, wxID_ANY, fname);

		// Tooltip showing full path
		label->SetToolTip(item);
		rowSizer->Add(label, 1, wxALIGN_CENTER_VERTICAL | wxLEFT, 5);

		// UP Button
		wxButton *btnUp = new wxButton(row, wxID_ANY, "^", wxDefaultPosition, wxSize(25, 20));
		btnUp->Enable(i > 0);
		btnUp->Bind(wxEVT_BUTTON, [=](wxCommandEvent &) {
			if (i > 0)
			{
				std::swap(currEdit->modFiles[i], currEdit->modFiles[i - 1]);
				RefreshModList(currEdit, listWindow, listSizer);
			}
		});
		rowSizer->Add(btnUp, 0, wxRIGHT | wxALIGN_CENTER_VERTICAL, 2);

		// DOWN Button
		wxButton *btnDown = new wxButton(row, wxID_ANY, "v", wxDefaultPosition, wxSize(25, 20));
		btnDown->Enable(i < currEdit->modFiles.size() - 1);
		btnDown->Bind(wxEVT_BUTTON, [=](wxCommandEvent &) {
			if (i < currEdit->modFiles.size() - 1)
			{
				std::swap(currEdit->modFiles[i], currEdit->modFiles[i + 1]);
				RefreshModList(currEdit, listWindow, listSizer);
			}
		});
		rowSizer->Add(btnDown, 0, wxRIGHT | wxALIGN_CENTER_VERTICAL, 2);

		// DELETE Button
		wxButton *btnDel = new wxButton(row, wxID_ANY, "X", wxDefaultPosition, wxSize(25, 20));
		btnDel->SetForegroundColour(*wxRED);
		btnDel->Bind(wxEVT_BUTTON, [=](wxCommandEvent &) {
			currEdit->modFiles.erase(currEdit->modFiles.begin() + i); // Remove from vector
			RefreshModList(currEdit, listWindow, listSizer);
		});
		rowSizer->Add(btnDel, 0, wxRIGHT | wxALIGN_CENTER_VERTICAL, 2);

		row->SetSizer(rowSizer);
		listSizer->Add(row, 0, wxEXPAND | wxALL, 1);
	}

	listWindow->Layout();
	listWindow->FitInside(); // Crucial for scrolling to update
}

void CreateAdvancedTab(Profile *currEdit, wxPanel *panel)
{
	wxBoxSizer *mainSizer = new wxBoxSizer(wxVERTICAL);

	// prepend parameters field
	wxStaticBoxSizer *pparamGroup = new wxStaticBoxSizer(wxVERTICAL, panel, "Prepend Additional Parameters");
	wxTextCtrl       *pparams = new wxTextCtrl(panel, wxID_ANY, "", wxDefaultPosition, wxSize(-1, 100), wxTE_MULTILINE,
	                                           wxTextValidator(wxFILTER_NONE, &currEdit->prependAdditionalParameters));
	pparamGroup->Add(pparams, 1, wxEXPAND);

	mainSizer->Add(pparamGroup, 1, wxEXPAND | wxALL, 10);

	// append parameters field
	wxStaticBoxSizer *aparamGroup = new wxStaticBoxSizer(wxVERTICAL, panel, "Append Additional Parameters");
	wxTextCtrl       *aparams = new wxTextCtrl(panel, wxID_ANY, "", wxDefaultPosition, wxSize(-1, 100), wxTE_MULTILINE,
	                                           wxTextValidator(wxFILTER_NONE, &currEdit->appendAdditionalParameters));
	aparamGroup->Add(aparams, 1, wxEXPAND);

	mainSizer->Add(aparamGroup, 1, wxEXPAND | wxALL, 10);

	panel->SetSizer(mainSizer);
}

void CreateOutputTab(Profile *currEdit, wxPanel *panel)
{
	// capture the radio button value manually and set values (beause wxValidator with radio buttons is not suitable)
	auto setupRadio = [currEdit](wxRadioButton *rb, int index) {
		if (currEdit->renderingBackend == index)
			rb->SetValue(true);
		rb->Bind(wxEVT_RADIOBUTTON, [currEdit, index](wxCommandEvent &) { currEdit->renderingBackend = index; });
	};

	wxBoxSizer *mainSizer = new wxBoxSizer(wxVERTICAL);

	// General Settings
	wxStaticBoxSizer *generalGroup = new wxStaticBoxSizer(wxVERTICAL, panel, "General");
	wxBoxSizer       *genRow       = new wxBoxSizer(wxHORIZONTAL);
	genRow->Add(new wxCheckBox(panel, wxID_ANY, "Fullscreen", wxDefaultPosition, wxDefaultSize, 0,
	                           wxGenericValidator(&currEdit->enableFullscreen)),
	            0, wxRIGHT, 15);
	genRow->Add(new wxCheckBox(panel, wxID_ANY, "Load Support WADs", wxDefaultPosition, wxDefaultSize, 0,
	                           wxGenericValidator(&currEdit->enableSupportWAD)),
	            0, wxRIGHT, 15);
	genRow->Add(new wxCheckBox(panel, wxID_ANY, "Disable Autoload", wxDefaultPosition, wxDefaultSize, 0,
	                           wxGenericValidator(&currEdit->disableAutoload)),
	            0);
	generalGroup->Add(genRow, 0, wxALL, 5);
	mainSizer->Add(generalGroup, 0, wxEXPAND | wxALL, 10);

	// Rendering API Group

	wxStaticBoxSizer *renderGroup = new wxStaticBoxSizer(wxVERTICAL, panel, "Rendering API");
	wxBoxSizer       *renderRow   = new wxBoxSizer(wxHORIZONTAL);

	wxRadioButton *vk = new wxRadioButton(panel, wxID_ANY, "Vulkan", wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
	wxRadioButton *gl = new wxRadioButton(panel, wxID_ANY, "OpenGL", wxDefaultPosition, wxDefaultSize);
	wxRadioButton *gles = new wxRadioButton(panel, wxID_ANY, "OpenGL ES", wxDefaultPosition, wxDefaultSize);

	renderRow->Add(vk,0, wxRIGHT, 15);
	renderRow->Add(gl, 0, wxRIGHT, 15);
	renderRow->Add(gles, 0);

	// setup bindings
	setupRadio(vk,0);
	setupRadio(gl,1);
	setupRadio(gles,2);

	renderGroup->Add(renderRow, 0, wxALL, 5);
	mainSizer->Add(renderGroup, 0, wxEXPAND | wxALL, 10);

	// Extra Graphics
	wxStaticBoxSizer *graphicsGroup = new wxStaticBoxSizer(wxVERTICAL, panel, "Extra Graphics");
	wxBoxSizer       *graphRow      = new wxBoxSizer(wxHORIZONTAL);
	graphRow->Add(new wxCheckBox(panel, wxID_ANY, "Lights", wxDefaultPosition, wxDefaultSize, 0,
	                             wxGenericValidator(&currEdit->enableLights)),
	              0, wxRIGHT, 15);
	graphRow->Add(new wxCheckBox(panel, wxID_ANY, "Brightmaps", wxDefaultPosition, wxDefaultSize, 0,
	                             wxGenericValidator(&currEdit->enableBrightmaps)),
	              0, wxRIGHT, 15);
	graphRow->Add(new wxCheckBox(panel, wxID_ANY, "Widescreen", wxDefaultPosition, wxDefaultSize, 0,
	                             wxGenericValidator(&currEdit->enableWidescreen)),
	              0);
	graphicsGroup->Add(graphRow, 0, wxALL, 5);
	mainSizer->Add(graphicsGroup, 0, wxEXPAND | wxALL, 10);

	panel->SetSizer(mainSizer);
}

void CreateFilesTab(Profile *currEdit, wxPanel *panel)
{
	wxBoxSizer *mainSizer = new wxBoxSizer(wxVERTICAL);

	// File Selection Area
	wxFlexGridSizer *grid = new wxFlexGridSizer(4, 2, 10, 10);
	grid->AddGrowableCol(1);

	// Config File Row
	grid->Add(new wxStaticText(panel, wxID_ANY, "Config File:"), 0, wxALIGN_CENTER_VERTICAL);
	wxBoxSizer *configBox = new wxBoxSizer(wxHORIZONTAL);

	wxTextCtrl *configTxt = new wxTextCtrl(panel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, 0,
	                                       wxTextValidator(wxFILTER_NONE, &currEdit->configFilePath));
	configTxt->SetInsertionPointEnd();

	wxButton *configButton = new wxButton(panel, wxID_ANY, "...", wxDefaultPosition, wxSize(30, -1));
	// bind it
	configButton->Bind(wxEVT_BUTTON, [panel, configTxt](wxCommandEvent &) {
		OpenPathPicker(panel, configTxt, "Select Config File", false, "INI files (*.ini)|*.ini");
	});
	configBox->Add(configTxt, 1, wxEXPAND | wxRIGHT, 5);
	configBox->Add(configButton, 0);
	grid->Add(configBox, 1, wxEXPAND);

	// Save Directory Row
	grid->Add(new wxStaticText(panel, wxID_ANY, "Save Directory:"), 0, wxALIGN_CENTER_VERTICAL);
	wxBoxSizer *saveBox = new wxBoxSizer(wxHORIZONTAL);

	wxTextCtrl *saveTxt = new wxTextCtrl(panel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, 0,
	                                     wxTextValidator(wxFILTER_NONE, &currEdit->saveDirPath));
	saveTxt->SetInsertionPointEnd();

	wxButton *saveDirButton = new wxButton(panel, wxID_ANY, "...", wxDefaultPosition, wxSize(30, -1));
	// bind it
	saveDirButton->Bind(wxEVT_BUTTON, [panel, saveTxt](wxCommandEvent &) {
		OpenPathPicker(panel, saveTxt, "Select Save Directory", true);
	});
	saveBox->Add(saveTxt, 1, wxEXPAND | wxRIGHT, 5);
	saveBox->Add(saveDirButton, 0);
	grid->Add(saveBox, 1, wxEXPAND);

	// Screenshot Dir Row
	grid->Add(new wxStaticText(panel, wxID_ANY, "Screenshot Dir:"), 0, wxALIGN_CENTER_VERTICAL);
	wxBoxSizer *shotBox = new wxBoxSizer(wxHORIZONTAL);

	wxTextCtrl *shotTxt = new wxTextCtrl(panel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, 0,
	                                     wxTextValidator(wxFILTER_NONE, &currEdit->screenshotDirPath));
	shotTxt->SetInsertionPointEnd();

	wxButton *scrnDirButton = new wxButton(panel, wxID_ANY, "...", wxDefaultPosition, wxSize(30, -1));
	// bind it
	scrnDirButton->Bind(wxEVT_BUTTON, [panel, shotTxt](wxCommandEvent &) {
		OpenPathPicker(panel, shotTxt, "Select Screenshot Directory", true);
	});
	shotBox->Add(shotTxt, 1, wxEXPAND | wxRIGHT, 5);
	shotBox->Add(scrnDirButton, 0);
	grid->Add(shotBox, 1, wxEXPAND);

	// Demo Directory Row
	grid->Add(new wxStaticText(panel, wxID_ANY, "Demo Directory:"), 0, wxALIGN_CENTER_VERTICAL);
	wxBoxSizer *demoBox = new wxBoxSizer(wxHORIZONTAL);

	wxTextCtrl *demoTxt = new wxTextCtrl(panel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, 0,
	                                     wxTextValidator(wxFILTER_NONE, &currEdit->demoDirPath));
	demoTxt->SetInsertionPointEnd();

	wxButton *demoDirButton = new wxButton(panel, wxID_ANY, "...", wxDefaultPosition, wxSize(30, -1));
	// bind it
	demoDirButton->Bind(wxEVT_BUTTON, [panel, demoTxt](wxCommandEvent &) {
		OpenPathPicker(panel, demoTxt, "Select Demo Directory", true);
	});
	demoBox->Add(demoTxt, 1, wxEXPAND | wxRIGHT, 5);
	demoBox->Add(demoDirButton, 0);
	grid->Add(demoBox, 1, wxEXPAND);

	mainSizer->Add(grid, 0, wxEXPAND | wxALL, 15);

	// Mods Header
	wxBoxSizer *modHeader = new wxBoxSizer(wxHORIZONTAL);
	modHeader->Add(new wxStaticText(panel, wxID_ANY, "Mods (additional .wad or .pk3 files)"), 1,
	               wxALIGN_CENTER_VERTICAL);

	wxButton *addModButton = new wxButton(panel, wxID_ANY, "+ Add Mod File");
	modHeader->Add(addModButton, 0);
	mainSizer->Add(modHeader, 0, wxEXPAND | wxLEFT | wxRIGHT, 15);

	// Mods List
	wxScrolledWindow *modList =
		new wxScrolledWindow(panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_SUNKEN);
	modList->SetScrollRate(0, 10);

	wxBoxSizer *modListSizer = new wxBoxSizer(wxVERTICAL);
	modList->SetSizer(modListSizer);

	addModButton->Bind(wxEVT_BUTTON, [currEdit, panel, modList, modListSizer](wxCommandEvent &) {
		wxFileDialog openFileDialog(panel, "Select Mod Files", wxGetCwd(), "",
		                            "Additional Mod Files (*.wad;*.pk3)|*.wad;*.pk3|All files (*.*)|*.*",
		                            wxFD_OPEN | wxFD_FILE_MUST_EXIST |
		                                wxFD_MULTIPLE); // this is a seperate case to allow for multiple selections

		if (openFileDialog.ShowModal() == wxID_CANCEL)
			return; // user clicked off -> stop

		wxArrayString paths;
		openFileDialog.GetPaths(paths);

		// Add selected paths to vector
		for (const wxString &path : paths)
		{
			currEdit->modFiles.push_back(path.ToStdString());
		}

		// Force Refresh the UI
		RefreshModList(currEdit, modList, modListSizer);
	});

	RefreshModList(currEdit, modList, modListSizer); // redraw the ui when user comes over

	mainSizer->Add(modList, 1, wxEXPAND | wxALL, 15);

	panel->SetSizer(mainSizer);
}

void CreateLaunchTab(Profile *currEdit, wxPanel *panel)
{
	wxBoxSizer *mainSizer = new wxBoxSizer(wxHORIZONTAL);

	// left side
	wxBoxSizer *leftCol = new wxBoxSizer(wxVERTICAL);

	wxStaticBoxSizer *launchModeGroup = new wxStaticBoxSizer(wxVERTICAL, panel, "Launch Mode");

	// capture the radio button value manually and set values (beause wxValidator with radio buttons is not suitable)
	auto setupRadio = [currEdit](wxRadioButton *rb, int index) {
		if (currEdit->launchParameters == index)
			rb->SetValue(true);
		rb->Bind(wxEVT_RADIOBUTTON, [currEdit, index](wxCommandEvent &) { currEdit->launchParameters = index; });
	};

	// Normal
	{
		wxBoxSizer    *row = new wxBoxSizer(wxHORIZONTAL);
		wxRadioButton *rb  = new wxRadioButton(panel, wxID_ANY, "Normal", wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
		rb->SetValue(true);
		row->Add(rb, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
		row->AddStretchSpacer();
		launchModeGroup->Add(row, 0, wxEXPAND | wxALL, 3);

		setupRadio(rb, 0);
	}

	// Map
	{
		wxBoxSizer    *row = new wxBoxSizer(wxHORIZONTAL);
		wxRadioButton *rb  = new wxRadioButton(panel, wxID_ANY, "Map");
		row->Add(rb, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
		row->AddStretchSpacer();
		wxSpinCtrl *mapSpinner = new wxSpinCtrl(panel, wxID_ANY, "1", wxDefaultPosition, wxDefaultSize, 0, 1, 100000);
		mapSpinner->SetValidator(wxGenericValidator(&currEdit->selectedLaunchMap));
		row->Add(mapSpinner, 0, wxALIGN_CENTER_VERTICAL);
		launchModeGroup->Add(row, 0, wxEXPAND | wxALL, 3);

		setupRadio(rb, 1);
	}

	// Savegame
	{
		wxBoxSizer    *row = new wxBoxSizer(wxHORIZONTAL);
		wxRadioButton *rb  = new wxRadioButton(panel, wxID_ANY, "Savegame");
		row->Add(rb, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
		row->AddStretchSpacer();
		wxTextCtrl *loadSavePath       = new wxTextCtrl(panel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, 0,
		                                                wxTextValidator(wxFILTER_NONE, &currEdit->selectedLaunchSave));
		wxButton   *loadSavePathButton = new wxButton(panel, wxID_ANY, "...", wxDefaultPosition, wxSize(30, -1));
		// bind it
		loadSavePathButton->Bind(wxEVT_BUTTON, [panel, loadSavePath](wxCommandEvent &) {
			OpenPathPicker(panel, loadSavePath, "Select Save File", false, "Zdoom Save files (*.zds)|*.zds");
		});
		row->Add(loadSavePath, 0, wxALIGN_CENTER_VERTICAL);
		row->Add(loadSavePathButton, 0);
		launchModeGroup->Add(row, 0, wxEXPAND | wxALL, 3);

		setupRadio(rb, 2);
	}

	// Demo Playback
	{
		wxBoxSizer    *row = new wxBoxSizer(wxHORIZONTAL);
		wxRadioButton *rb  = new wxRadioButton(panel, wxID_ANY, "Play Demo");
		row->Add(rb, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
		row->AddStretchSpacer();
		wxTextCtrl *playDemPath       = new wxTextCtrl(panel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, 0,
		                                               wxTextValidator(wxFILTER_NONE, &currEdit->selectedLaunchDemoPlayback));
		wxButton   *playDemPathButton = new wxButton(panel, wxID_ANY, "...", wxDefaultPosition, wxSize(30, -1));
		// bind it
		playDemPathButton->Bind(wxEVT_BUTTON, [panel, playDemPath](wxCommandEvent &) {
			OpenPathPicker(panel, playDemPath, "Select Demo File", false, "Demo Lump files (*.lmp)|*.lmp");
		});
		row->Add(playDemPath, 0, wxALIGN_CENTER_VERTICAL);
		row->Add(playDemPathButton, 0);
		launchModeGroup->Add(row, 0, wxEXPAND | wxALL, 3);

		setupRadio(rb, 3);
	}

	// Demo Record
	{
		wxBoxSizer    *row = new wxBoxSizer(wxHORIZONTAL);
		wxRadioButton *rb  = new wxRadioButton(panel, wxID_ANY, "Record Demo");
		row->Add(rb, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
		row->AddStretchSpacer();
		wxTextCtrl *recDemPath       = new wxTextCtrl(panel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, 0,
		                                              wxTextValidator(wxFILTER_NONE, &currEdit->selectedLaunchDemoRecord));
		wxButton   *recDemPathButton = new wxButton(panel, wxID_ANY, "...", wxDefaultPosition, wxSize(30, -1));
		// bind it
		recDemPathButton->Bind(wxEVT_BUTTON, [panel, recDemPath](wxCommandEvent &) {
			OpenPathPicker(panel, recDemPath, "Select Demo File", false, "Demo Lump files (*.lmp)|*.lmp");
		});
		row->Add(recDemPath, 0, wxALIGN_CENTER_VERTICAL);
		row->Add(recDemPathButton, 0);
		launchModeGroup->Add(row, 0, wxEXPAND | wxALL, 3);

		setupRadio(rb, 4);
	}

	leftCol->Add(launchModeGroup, 0, wxEXPAND | wxALL, 5);

	// Difficulty settings
	wxStaticBoxSizer *gameplayGroup = new wxStaticBoxSizer(wxVERTICAL, panel, "Gameplay");

	// Skill select
	wxBoxSizer *skillRow = new wxBoxSizer(wxHORIZONTAL);
	skillRow->Add(new wxStaticText(panel, wxID_ANY, "Skill:"), 1, wxALIGN_CENTER_VERTICAL);

	wxComboBox *skillCombo = new wxComboBox(panel, wxID_ANY, "3 - Medium", wxDefaultPosition, wxDefaultSize, 0, NULL, 0,
	                                        wxGenericValidator(&currEdit->difficultySkillRating));
	skillCombo->Append("1 - Baby");
	skillCombo->Append("2 - Easy");
	skillCombo->Append("3 - Medium");
	skillCombo->Append("4 - Hard");
	skillCombo->Append("5 - Nightmare");
	skillCombo->SetEditable(false);

	skillRow->Add(skillCombo, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
	gameplayGroup->Add(skillRow, 0, wxEXPAND | wxALL, 5);

	gameplayGroup->AddSpacer(5);

	// Checkboxes for modifiers
	long chkFlags = wxLEFT | wxBOTTOM; // add space between checkboxes for better look
	gameplayGroup->Add(new wxCheckBox(panel, wxID_ANY, "Fast Monsters", wxDefaultPosition, wxDefaultSize, 0,
	                                  wxGenericValidator(&currEdit->difficultyFastMonsters)),
	                   0, chkFlags, 5);
	gameplayGroup->Add(new wxCheckBox(panel, wxID_ANY, "Respawn Monsters", wxDefaultPosition, wxDefaultSize, 0,
	                                  wxGenericValidator(&currEdit->difficultyRespawnMonsters)),
	                   0, chkFlags, 5);
	gameplayGroup->Add(new wxCheckBox(panel, wxID_ANY, "No Monsters", wxDefaultPosition, wxDefaultSize, 0,
	                                  wxGenericValidator(&currEdit->difficultyNoMonsters)),
	                   0, chkFlags, 5);

	wxButton *advGameplayButton = new wxButton(panel, wxID_ANY, "More ...");
	gameplayGroup->Add(advGameplayButton, 0, wxALIGN_LEFT | wxEXPAND); // botton for more options
	// bind it
	advGameplayButton->Bind(wxEVT_BUTTON, [currEdit, panel](wxCommandEvent &) { advGameplay(currEdit, panel); });
	leftCol->Add(gameplayGroup, 0, wxEXPAND | wxALL, 5);

	// right side
	wxBoxSizer *rightCol = new wxBoxSizer(wxVERTICAL);

	// Remote Multiplayer
	wxStaticBoxSizer *remoteGroup = new wxStaticBoxSizer(wxVERTICAL, panel, "Remote Multiplayer");
	wxFlexGridSizer  *remoteGrid  = new wxFlexGridSizer(3, 2, 5, 5);
	remoteGrid->AddGrowableCol(1);

	remoteGrid->Add(new wxStaticText(panel, wxID_ANY, "Remote Address:"), 0, wxALIGN_CENTER_VERTICAL);
	remoteGrid->Add(new wxTextCtrl(panel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, 0,
	                               wxTextValidator(wxFILTER_NONE, &currEdit->joinAddress)),
	                0, wxEXPAND);
	remoteGrid->Add(new wxStaticText(panel, wxID_ANY, "Remote Port (Default 5029):"), 0, wxALIGN_CENTER_VERTICAL);
	remoteGrid->Add(new wxTextCtrl(panel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, 0,
	                               wxTextValidator(wxFILTER_NONE, &currEdit->joinPort)),
	                0, wxEXPAND);
	remoteGrid->Add(new wxStaticText(panel, wxID_ANY, "Team No. (255 = Random)"), 0, wxALIGN_CENTER_VERTICAL);
	remoteGrid->Add(new wxTextCtrl(panel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, 0,
	                               wxTextValidator(wxFILTER_NONE, &currEdit->joinTeamNo)),
	                0, wxEXPAND);

	remoteGroup->Add(remoteGrid, 1, wxEXPAND | wxALL, 5);
	rightCol->Add(remoteGroup, 0, wxEXPAND | wxALL, 5);

	// Host Multiplayer
	wxStaticBoxSizer *hostGroup = new wxStaticBoxSizer(wxVERTICAL, panel, "Host Multiplayer");
	wxFlexGridSizer  *hostGrid  = new wxFlexGridSizer(5, 2, 5, 5);
	hostGrid->AddGrowableCol(1);

	hostGrid->Add(new wxStaticText(panel, wxID_ANY, "Host Port (Default 5029):"), 0, wxALIGN_CENTER_VERTICAL);
	hostGrid->Add(new wxTextCtrl(panel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, 0,
	                             wxTextValidator(wxFILTER_NONE, &currEdit->hostPort)),
	              0, wxEXPAND);

	hostGrid->Add(new wxStaticText(panel, wxID_ANY, "Max Players:"), 0, wxALIGN_CENTER_VERTICAL);
	wxSpinCtrl *playerCounterSpin = new wxSpinCtrl(panel, wxID_ANY, "8", wxDefaultPosition, wxDefaultSize, 0, 1, 64);
	playerCounterSpin->SetValidator(wxGenericValidator(&currEdit->hostMaxPlayers));
	hostGrid->Add(playerCounterSpin, 0, wxEXPAND);

	hostGrid->Add(new wxStaticText(panel, wxID_ANY, "Tickrate:"), 0, wxALIGN_CENTER_VERTICAL);

	wxComboBox *TickRateCombo = new wxComboBox(panel, wxID_ANY, "25Hz", wxDefaultPosition, wxDefaultSize, 0, NULL, 0,
	                                           wxGenericValidator(&currEdit->hostTickRate));
	TickRateCombo->Append("25Hz");
	TickRateCombo->Append("17.5Hz");
	TickRateCombo->Append("11.6Hz");
	TickRateCombo->SetEditable(false);
	hostGrid->Add(TickRateCombo, 0, wxEXPAND);

	hostGrid->Add(new wxStaticText(panel, wxID_ANY, "Gamemode:"), 0, wxALIGN_CENTER_VERTICAL);
	wxComboBox *gameModeCombo = new wxComboBox(panel, wxID_ANY, "Cooperative", wxDefaultPosition, wxDefaultSize, 0,
	                                           NULL, 0, wxGenericValidator(&currEdit->hostGamemode));
	gameModeCombo->Append("Cooperative");
	gameModeCombo->Append("Team Deathmatch");
	gameModeCombo->Append("Alt. Team Deathmatch");
	gameModeCombo->Append("Deathmatch");
	gameModeCombo->Append("Alt. Deathmatch");
	gameModeCombo->SetEditable(false);
	hostGrid->Add(gameModeCombo, 0, wxEXPAND);

	hostGrid->Add(new wxStaticText(panel, wxID_ANY, "Network Mode:"), 0, wxALIGN_CENTER_VERTICAL);
	wxComboBox *netModeCombo = new wxComboBox(panel, wxID_ANY, "Peer-to-Peer", wxDefaultPosition, wxDefaultSize, 0,
	                                          NULL, 0, wxGenericValidator(&currEdit->hostNetworkMode));
	netModeCombo->Append("Peer-to-Peer");
	netModeCombo->Append("Packet Server");
	netModeCombo->SetEditable(false);
	hostGrid->Add(netModeCombo, 0, wxEXPAND);

	hostGroup->Add(hostGrid, 1, wxEXPAND | wxALL, 5);
	rightCol->Add(hostGroup, 0, wxEXPAND | wxALL, 5);

	// Compatibility
	wxStaticBoxSizer *compatGroup = new wxStaticBoxSizer(wxVERTICAL, panel, "Compatibility");
	wxFlexGridSizer  *compatGrid  = new wxFlexGridSizer(2, 2, 5, 5);
	compatGrid->AddGrowableCol(1);

	compatGrid->Add(new wxStaticText(panel, wxID_ANY, "Compatibility preset:"), 0, wxALIGN_CENTER_VERTICAL);
	wxComboBox *complevelCombo = new wxComboBox(panel, wxID_ANY, "0 - Default", wxDefaultPosition, wxDefaultSize, 0,
	                                            NULL, 0, wxGenericValidator(&currEdit->compatLevel));
	complevelCombo->Append("0 - Default");
	complevelCombo->Append("1 - Doom");
	complevelCombo->Append("2 - Doom (Strict)");
	complevelCombo->Append("3 - Boom");
	complevelCombo->Append("4 - Boom (Strict)");
	complevelCombo->Append("5 - MBF");
	complevelCombo->Append("6 - MBF (Strict)");
	complevelCombo->Append("7 - MBF21");
	complevelCombo->Append("8 - MBF21 (Strict)");
	complevelCombo->Append("9 - ZDoom 2.0.63");
	complevelCombo->SetEditable(false);
	compatGrid->Add(complevelCombo, 0, wxALIGN_TOP | wxALIGN_RIGHT | wxTOP, 5);

	wxBoxSizer *compStack = new wxBoxSizer(wxVERTICAL);
	compatGrid->Add(compStack, 1, wxEXPAND);

	compatGroup->Add(compatGrid, 1, wxEXPAND | wxALL, 5);
	wxButton *advComButton = new wxButton(panel, wxID_ANY, "Custom ...");
	compatGroup->Add(advComButton, 0, wxALIGN_LEFT | wxEXPAND);
	// bind it
	advComButton->Bind(wxEVT_BUTTON, [currEdit, panel](wxCommandEvent &) { advCompat(currEdit, panel); });
	rightCol->Add(compatGroup, 0, wxEXPAND | wxALL, 5);

	mainSizer->Add(leftCol, 1, wxEXPAND | wxALL, 5);
	mainSizer->Add(rightCol, 1, wxEXPAND | wxALL, 5);

	panel->SetSizer(mainSizer);
}

void CreateGeneralTab(Profile *currEdit, wxPanel *panel)
{

	wxBoxSizer *pageSizer = new wxBoxSizer(wxVERTICAL);

	wxGridBagSizer *gbSizer = new wxGridBagSizer(10, 20);

	gbSizer->Add(new wxStaticText(panel, wxID_ANY, "Title:"), wxGBPosition(0, 0), wxGBSpan(1, 1),
	             wxALIGN_CENTER_VERTICAL);
	gbSizer->Add(new wxTextCtrl(panel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, 0,
	                            wxTextValidator(wxFILTER_NONE, &currEdit->title)),
	             wxGBPosition(0, 1), wxGBSpan(1, 1), wxEXPAND);

	gbSizer->Add(new wxStaticText(panel, wxID_ANY, "Author:"), wxGBPosition(1, 0), wxGBSpan(1, 1),
	             wxALIGN_CENTER_VERTICAL);
	gbSizer->Add(new wxTextCtrl(panel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, 0,
	                            wxTextValidator(wxFILTER_NONE, &currEdit->author)),
	             wxGBPosition(1, 1), wxGBSpan(1, 1), wxEXPAND);

	gbSizer->Add(new wxStaticText(panel, wxID_ANY, "Release Date:"), wxGBPosition(2, 0), wxGBSpan(1, 1),
	             wxALIGN_CENTER_VERTICAL);
	gbSizer->Add(new wxTextCtrl(panel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, 0,
	                            wxTextValidator(wxFILTER_NONE, &currEdit->releaseDate)),
	             wxGBPosition(2, 1), wxGBSpan(1, 1), wxEXPAND);

	wxFlexGridSizer *rightSizer = new wxFlexGridSizer(2, 10, 10);
	rightSizer->AddGrowableCol(1);
	rightSizer->Add(new wxStaticText(panel, wxID_ANY, "Type:"), 0, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);

	wxComboBox *typeCombo =
		new wxComboBox(panel, wxID_ANY, currEdit->isIWAD == 1 ? "IWAD" : "(P)WAD", wxDefaultPosition, wxDefaultSize, 0,
	                   NULL, 0, wxGenericValidator(&currEdit->isIWAD));
	typeCombo->Append("(P)WAD");
	typeCombo->Append("IWAD");
	typeCombo->SetEditable(false);
	rightSizer->Add(typeCombo, 0, wxEXPAND);

	// always show IWAD row
	rightSizer->Add(new wxStaticText(panel, wxID_ANY, "IWAD:"), 0, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);

	wxBoxSizer *iwadBox = new wxBoxSizer(wxHORIZONTAL);
	wxTextCtrl *iwadTxt = new wxTextCtrl(panel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, 0,
	                                     wxTextValidator(wxFILTER_NONE, &currEdit->iwadFilePath));
	iwadBox->Add(iwadTxt, 1, wxEXPAND | wxRIGHT, 5);
	wxButton *iwadButton = new wxButton(panel, wxID_ANY, "...", wxDefaultPosition, wxSize(30, -1));
	iwadBox->Add(iwadButton, 0);
	// bind it
	iwadButton->Bind(wxEVT_BUTTON, [panel, iwadTxt](wxCommandEvent &) {
		OpenPathPicker(panel, iwadTxt, "Select IWAD File", false, "WAD files (*.wad)|*.wad");
	});
	rightSizer->Add(iwadBox, 0, wxEXPAND);

	// adds the PWAD row, which will be conditionally shown/hidden
	wxStaticText *pwadLabel = new wxStaticText(panel, wxID_ANY, "(P)WAD:");
	rightSizer->Add(pwadLabel, 0, wxALIGN_CENTER_VERTICAL | wxALIGN_RIGHT);

	wxBoxSizer *pwadBox = new wxBoxSizer(wxHORIZONTAL);
	wxTextCtrl *pwadTxt = new wxTextCtrl(panel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, 0,
	                                     wxTextValidator(wxFILTER_NONE, &currEdit->pwadFilePath));
	pwadBox->Add(pwadTxt, 1, wxEXPAND | wxRIGHT, 5);
	wxButton *pwadButton = new wxButton(panel, wxID_ANY, "...", wxDefaultPosition, wxSize(30, -1));
	pwadBox->Add(pwadButton, 0);
	// bind it
	pwadButton->Bind(wxEVT_BUTTON, [panel, pwadTxt](wxCommandEvent &) {
		OpenPathPicker(panel, pwadTxt, "Select PWAD File", false, "WAD files (*.wad)|*.wad");
	});
	rightSizer->Add(pwadBox, 0, wxEXPAND);

	bool isPwad = (typeCombo->GetValue() == "(P)WAD");
	pwadLabel->Show(isPwad);
	pwadBox->ShowItems(isPwad);
	gbSizer->Add(rightSizer, wxGBPosition(0, 3), wxGBSpan(3, 2), wxEXPAND | wxLEFT, 20);

	// logic to show/hide the PWAD row based on selection
	typeCombo->Bind(wxEVT_COMBOBOX, [=](wxCommandEvent &) {
		panel->Freeze();

		bool showPwad = (typeCombo->GetValue() == "(P)WAD");
		pwadLabel->Show(showPwad);
		pwadBox->ShowItems(showPwad);

		panel->Layout(); // Recalculate positions because update
		panel->Thaw();
	});

	gbSizer->AddGrowableCol(1);
	gbSizer->AddGrowableCol(4);

	pageSizer->Add(gbSizer, 0, wxEXPAND | wxALL, 20);

	// Description Area
	wxBoxSizer   *descSizer = new wxBoxSizer(wxHORIZONTAL);
	wxStaticText *descLabel = new wxStaticText(panel, wxID_ANY, "Description:");
	wxTextCtrl   *descText  = new wxTextCtrl(panel, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE,
	                                         wxTextValidator(wxFILTER_NONE, &currEdit->description));

	descSizer->Add(descLabel, 0, wxTOP | wxRIGHT, 5);
	descSizer->Add(descText, 1, wxEXPAND);
	pageSizer->Add(descSizer, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 20);

	panel->SetSizer(pageSizer);
}

void ProfileSettings::ProfileSettingsMenu(wxWindow *parent, const wxString &title, const std::string &profilePath)
{
	// the profile we are editing
	Profile *currEdit = new Profile();

	// populate the Profile from filepath
	currEdit->loadFromFile(profilePath);

	// we have read it, now fill in all fields

	// create the window here since above is not a constructor
	this->Create(parent, wxID_ANY, "Profile Settings", wxDefaultPosition, wxSize(850, 650));

	this->SetExtraStyle(GetExtraStyle() | wxWS_EX_VALIDATE_RECURSIVELY);

	this->SetWindowStyle(wxDEFAULT_DIALOG_STYLE &
	                     ~(wxRESIZE_BORDER | wxMAXIMIZE_BOX)); // ban the user from resizing and maximizing

	wxBoxSizer *mainSizer   = new wxBoxSizer(wxVERTICAL); // Main vertical sizer so fit everything
	wxBoxSizer *buttonSizer = new wxBoxSizer(wxHORIZONTAL);

	wxNotebook *tabber = new wxNotebook(this, wxID_ANY); // Used for the tabs in the settings menu

	// Generate every tab panel
	wxPanel *generalPanel = new wxPanel(tabber);
	CreateGeneralTab(currEdit, generalPanel);
	wxPanel *launchPanel = new wxPanel(tabber);
	CreateLaunchTab(currEdit, launchPanel);
	wxPanel *filesPanel = new wxPanel(tabber);
	CreateFilesTab(currEdit, filesPanel);
	wxPanel *outputPanel = new wxPanel(tabber);
	CreateOutputTab(currEdit, outputPanel);
	wxPanel *advancedPanel = new wxPanel(tabber);
	CreateAdvancedTab(currEdit, advancedPanel);

	// finer error checking for validation fails
	if (!generalPanel->TransferDataToWindow())
		wxMessageBox("Error in General Tab");
	else if (!launchPanel->TransferDataToWindow())
		wxMessageBox("Error in Launch Tab");
	else if (!filesPanel->TransferDataToWindow())
		wxMessageBox("Error in Files Tab");
	else if (!outputPanel->TransferDataToWindow())
		wxMessageBox("Error in Output Tab");
	else if (!advancedPanel->TransferDataToWindow())
		wxMessageBox("Error in Advanced Tab");

	// add them to the tabber
	tabber->AddPage(generalPanel, "General");
	tabber->AddPage(launchPanel, "Launch");
	tabber->AddPage(filesPanel, "Files");
	tabber->AddPage(outputPanel, "Output");
	tabber->AddPage(advancedPanel, "Advanced");

	mainSizer->Add(tabber, 1, wxEXPAND | wxALL, 5);

	// Buttons at the bottom of the dialog
	wxButton *DeleteButton = new wxButton(this, wxID_ANY, "Delete Profile ...");

	wxButton *SaveButton = new wxButton(this, wxID_ANY, "Save and Return");

	// bind an event to save button
	SaveButton->Bind(wxEVT_BUTTON, [currEdit, this, profilePath](wxCommandEvent &) {
		if (this->TransferDataFromWindow())
		{
			try
			{
				currEdit->saveToFile(profilePath);

				delete currEdit;
				this->EndModal(wxID_OK); // all fine? go back
			}
			catch (const std::exception &e)
			{
				wxMessageBox(e.what(), "Error Saving", wxICON_ERROR);
			}
		}
		else
		{
			// validation fail
			wxMessageBox("Invalid inputs. Please check your inputs.", "Error", wxICON_ERROR);
		}
	});

	DeleteButton->Bind(wxEVT_BUTTON, [currEdit, this, profilePath](wxCommandEvent &) {
		// ask the user , could be an accidental misclick
		wxMessageDialog check(this,
		                      "Are you sure?\nThis will permanently delete the profile and all files in its "
		                      "directory.\n\nThis cannot be undone.",
		                      "Confirm Deletion", wxYES_NO | wxICON_WARNING | wxNO_DEFAULT);

		if (check.ShowModal() == wxID_YES)
		{
			std::filesystem::path fileP(profilePath);
			std::filesystem::path dirToWipe = fileP.parent_path();

			// Ensure we have a valid parent directory
			if (!dirToWipe.empty() && std::filesystem::exists(dirToWipe))
			{
				std::filesystem::remove_all(dirToWipe); // fully scrub it

				delete currEdit;
				this->EndModal(wxID_REMOVE); // pass this back to the main window to signal deletion
			}
			else
				wxMessageBox("Could not determine directory to delete.", "Error", wxICON_ERROR);
		}
	});

	// tidy it up into the sizer
	buttonSizer->Add(DeleteButton, 0, wxALIGN_CENTER_VERTICAL);
	buttonSizer->AddStretchSpacer();
	buttonSizer->Add(SaveButton, 0, wxALIGN_CENTER_VERTICAL);

	mainSizer->Add(buttonSizer, 0, wxEXPAND | wxALL, 10);

	this->SetSizer(mainSizer);
}
