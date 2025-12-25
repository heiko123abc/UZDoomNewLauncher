#include "about.h"

void About::ReleaseNotesDialog(wxWindow *parent)
{

	// create the window here since above is not a constructor
	this->Create(parent, wxID_ANY, "Release Notes", wxDefaultPosition, parent->FromDIP(wxSize(1000, 800)));

	// hardcoded patch notes (for now), supports basic HTML formatting
	wxString patchNotes = wxString::FromUTF8("UZDoom version 4.14.2, released 2025-05-03<br><br>"

						  "This update delivers various bug fixes, performance optimizations, and significantly "
	                      "expands modding capabilities.<br><br>"

						  "- Ortho + OOB fixes and improvements<br>"
						  "- Revert clipper to older code path when not in Ortho / OOB(speed improvement)<br>"
						  "- Fix to some crashes and memory leaks<br>"
						  "- Exposed DDoor to ZScript, Exposed DPlat to ZScript, Exposed more of the Ceiling thinker, "
	                      "Exposed more of the Floor thinker, Exposed DElevator to ZScript.<br>"
						  "- Exported: GetLumpContainer, GetContainerName, GetLumpFullPath for WADS struct, useful for "
	                      "debugging custom - made parsers and identifying where problems may arise.<br>"
						  "- Added autoSwitch parameter to A_ReFire<br>"
						  "- add a few commonly - used gzdoom - specific properties to the dehacked parser<br>"
						  "- many more fixes and improvements<br><br>"

						  "For more details see : https://forum.zdoom.org/viewtopic.php?t=80447");

	// required to display rich text
	wxHtmlWindow *htmlWin = new wxHtmlWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHW_SCROLLBAR_AUTO);
	htmlWin->SetPage(patchNotes);

	// The checkbox to show/hide patch notes on update
	wxCheckBox *showOnUpdateReq = new wxCheckBox(this, wxID_ANY, "Show these notes upon new update");
	showOnUpdateReq->SetValue(true); // Default to checked

	// Button to close the dialog
	wxButton *closeButton = new wxButton(this, wxID_OK, "Close");

	// Layout using a vertical box sizer for proper arrangement
	wxBoxSizer *vbox = new wxBoxSizer(wxVERTICAL);
	vbox->Add(htmlWin, 1, wxEXPAND | wxALL, 10);
	vbox->Add(showOnUpdateReq, 0, wxALIGN_LEFT | wxLEFT | wxBOTTOM, 10);
	vbox->Add(closeButton, 0, wxALIGN_CENTER | wxALL, 10);

	SetSizer(vbox);
	Layout();

	Center(); // force everything to center
}

void About::CreditsDialog(wxWindow *parent)
{

	// create the window here since above is not a constructor
	this->Create(parent, wxID_ANY, "Credits", wxDefaultPosition,  parent->FromDIP(wxSize(1000, 800)));

	// hardcoded credits (for now), supports basic HTML formatting
	wxString credits = wxString::FromUTF8(R"(<html>
<body text="#000000" bgcolor="#FFFFFF" link="#0000FF" vlink="#0000FF">

<center>
    <h1>UZDoom</h1>
    <p>https://zdoom.org/</p>
</center>

<p><b>UZDoom</b> is a modern, feature-rich source port for the classic game DOOM. A continuation of ZDoom and GZDoom, UZDoom enhances the original DOOM engine, providing advanced features like high-resolution graphics, dynamic lighting, 3D floors, and extensive modding support for modern operating systems.</p>

<p>UZDoom is free and open-source software, built and maintained by a dedicated community of developers and enthusiasts.</p>

<hr>

<h2>Acknowledgments</h2>
<p>UZDoom would not be possible without the work of many people. We extend our immense gratitude to:</p>

<ul>
    <li><b>id Software</b> for creating the original DOOM and releasing its source code.</li>
    <li><b>Marisa Heit</b> for her foundational work on ZDoom, and <b>Christoph Oelckers</b> for his work on GZDoom.</li>
    <li>The countless modders, mappers, and artists in the DOOM community who continue to create amazing content.</li>
    <li>All the contributors who have submitted code, reported bugs, and helped improve the project over the years.</li>
</ul>

<p><i>The UZDoom Icon was designed by Carlos "Cardboard Marty" Sanchez, copyrighted to the UZDoom team, licensed under Creative Commons BY-SA 4.0</i></p>

<hr>

<h2>Licensing</h2>
<font size="-1">
    <p>
        Copyright 1993-1996 id Software<br>
        Copyright 1999-2016 Marisa Heit<br>
        Copyright 2002-2016 Christoph Oelckers<br>
        Copyright 2017-2025 GZDoom Maintainers and Contributors<br>
        Copyright 2025 UZDoom Maintainers and Contributors
    </p>

    <p>This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.</p>

    <p>This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.</p>

    <p>You should have received a copy of the GNU General Public License along with this program. If not, see http://www.gnu.org/licenses/</p>
</font>

<hr>

<h2>Contributors</h2>
<font size="-1">
    Marisa "Randi" Heit<br>
    Andy "darkknight" Baker<br>
    Andy "Aurikan" Kempling<br>
    Martin Howe<br>
    Rand "grayman" Phares<br>
    Lee Killough<br>
    Papst Johannes Jörg IV<br>
    John Cole<br>
    Jeffrey Cuenco<br>
    Ty Halderman<br>
    Jan "Hirogen2" Engelhardt<br>
    Stevie-O<br>
    Vladimir Arnost<br>
    Ethan "GooberMan" Watson<br>
    Mike B.<br>
    Tim "Timmie" Stump<br>
    Christoph "Graf Zahl" Oelckers<br>
    Jim<br>
    Chris Robinson<br>
    zloba<br>
    Justin<br>
    Nigel "Enjay" Rowand<br>
    James "Quasar" Haley<br>
    Jan "Grubber" Cholasta<br>
    Michael "Necromage" Weber<br>
    Paul Hsieh<br>
    Karate Chris<br>
    Kaitlyn "Kate" Fox<br>
    Braden "Blzut3" Obrzut<br>
    Thomas<br>
    Andrey "entryway" Budko<br>
    Gez<br>
    Matthew "doom2day" Gilzinger<br>
    Simon "Fraggle" Howard<br>
    CO2<br>
    Xaser "The Conquerer" Acheron<br>
    Spleen<br>
    Rachael "Eruanna" Alexanderson<br>
    bagheadspidey<br>
    SnailMan<br>
    NeuralStunner<br>
    Lexi "LexiMax" Mayfield<br>
    Alexey "_mental_" Lysiuk<br>
    Chris Robinson<br>
    arezey<br>
    Edoardo Prezioso<br>
    Alex Qyoun-ae<br>
    Edward "Edward850" Richardson<br>
    Mike "Chungy" Swanson<br>
    Teemu Piippo<br>
    galtgendo<br>
    Shawn Walker-Salas<br>
    WChrisK<br>
    darealshinji<br>
    Leonard<br>
    Ralgor<br>
    MajorCooke<br>
    Dennis "Exl" Meuwissen<br>
    John Palomo Jr<br>
    fdari<br>
    ChillyDoom<br>
    Alexey Khokholov<br>
    Kyle Evans<br>
    ZZYZX<br>
    Blue Shadow<br>
    Nash Muhandes<br>
    Benjamin "The Zombie Killer" Moir<br>
    ZzZombo<br>
    vanhofen<br>
    Chronos Ouroboros<br>
    svdijk<br>
    Lexy "Eevee" Munroe<br>
    Alison "Marrub" Sanderson<br>
    Christopher Bruns<br>
    Tuomas Virtanen<br>
    Chris Spiegel<br>
    Christopher Snowhill<br>
    nukeykt<br>
    MaxED<br>
    DaMan<br>
    Michael Labbe<br>
    Jason Yundt<br>
    Magnus "dpJudas" Norddahl<br>
    Roadcrosser<br>
    subenji<br>
    Jordon "Striker" Moss<br>
    yqco<br>
    arookas<br>
    LordMisfit<br>
    N.E.C<br>
    FishyClockwork<br>
    WilliamFeely<br>
    Thomas Hume<br>
    Matthew McAllister<br>
    Robert Cochran<br>
    Dmitri Kourennyi<br>
    Dugan Chen<br>
    Kirill Gavrilov Tartynskih<br>
    Jean-Paul "J.P." LeBreton<br>
    David Carlier<br>
    Kevin Caccamo<br>
    Gutawer<br>
    James "Jimmy" Paddock<br>
    AraHaan<br>
    AntiBlueQuirk<br>
    Henk Roos<br>
    Kostov<br>
    Jameson Ernst<br>
    Marisa "the Magician" Lago<br>
    Neil McPhail<br>
    Alexander Wilms<br>
    InsanityBringer<br>
    Diego "drfrag666" Antonio de Membiela Sánchez<br>
    Vitaly "Wohlstand" Novichkov<br>
    Jason Francis<br>
    Zain Aamer<br>
    Hisymak<br>
    lucinda lovebuny<br>
    RockstarRaccoon<br>
    Cacodemon345<br>
    Erick Tenorio<br>
    Alexander Kromm (m8f)<br>
    argv-minus-one<br>
    usernameak<br>
    Sterling "Caligari87" Parker<br>
    player701<br>
    Tommy Nguyen<br>
    SanyaWaffles<br>
    Ijon<br>
    Timo Myyrä<br>
    Piotr Kubaj<br>
    Arless Hill<br>
    mc776<br>
    XxMiltenXx<br>
    William Breathitt Gray<br>
    Danilo Spinella<br>
    cybermind<br>
    Dzmitry Malyshau<br>
    3saster<br>
    Kevin Hutchins<br>
    Patryk Obara<br>
    cuttlefish<br>
    Hugo Locurcio<br>
    Petr Kobalicek<br>
    Colton G. Rushton<br>
    hdr88<br>
    Dominus Iniquitatis<br>
    Ed the Bat<br>
    Fabian Greffrath<br>
    Petr Mrázek<br>
    svenhoefer<br>
    johannes hanika<br>
    Blue<br>
    Skepticist<br>
    Mekboss<br>
    Mitch Richters<br>
    Farkas Péter<br>
    Perry Fraser<br>
    Maarten Lensink<br>
    AFADoomer<br>
    Kyle Johnson<br>
    Vidar Flesjø<br>
    NY00123<br>
    Igor Molchanov<br>
    Jaime Moreira<br>
    C.W. Betts<br>
    Nikolay Ambartsumov<br>
    azamorapl<br>
    Emily<br>
    William E. Waterman<br>
    NukiRaccoon<br>
    Erick "Erick194" Vásquez García<br>
    DarkOK<br>
    Chernoskill<br>
    Timothy Quinn<br>
    James Le Cuirot<br>
    sgrunt<br>
    Leonid "Dasperal" Murin<br>
    Shiny Metagross<br>
    Kaelan "kevansevans" Evans<br>
    Jeroen de Baat<br>
    arrowgent<br>
    Yarn366<br>
    Emile Belanger<br>
    Sean Baggaley<br>
    RaveYard<br>
    atsb<br>
    inkoalawetrust<br>
    Emanuele Disco<br>
    Lippeth<br>
    temx<br>
    Jay<br>
    Sally "TehRealSalt" Cochenour<br>
    Omar Polo<br>
    Markus Wölkchen<br>
    crashmahoney<br>
    Yukita Mayako<br>
    Boondorl<br>
    Uni Musuotankarep<br>
    CandiceJoy<br>
    John Stebbins<br>
    Chris "Macil" Cowan<br>
    l2ksolkov<br>
    Jacob Alexander Tice<br>
    Joshua Watt<br>
    Romain Tisserand<br>
    Agent_Ash (jekyllgrim)<br>
    Brad Smith<br>
    &amp;Olga<br>
    Tyler Schneider<br>
    Ignacio Taranto<br>
    Vasilii Shirokii<br>
    Andrey Shustov<br>
    Dileep V. Reddy<br>
    Ștefan Talpalaru<br>
    Ru5tK1ng<br>
    DyNaM1Kk<br>
    Kartinea<br>
    Acts 19 quiz<br>
    Ashley "Dark Assassin" Miller-Jelfs<br>
    Vitaliy Kanev<br>
    Ritchie Swann<br>
    TwelveEyes<br>
    HHonzik<br>
    Hexadec<br>
    Jon Heard<br>
    BinarryCode<br>
    Kevin "Eonfge" Degeling<br>
    Adam Kaminski<br>
    Jon Daniel<br>
    f7cjo<br>
    Peppersawce<br>
    XLightningStormL<br>
    Owlet7<br>
    TheSuperDave938<br>
    biwa<br>
    Robert Godward<br>
    Marcus Minhorst<br>
    VileCornstarch<br>
    Florian Piesche<br>
    Nikita Lita<br>
    SAN4EZ DREAMS<br>
    Madeline Mewmews<br>
    rafapaezbas<br>
    BinaryCode<br>
    dwing4g<br>
    Moises Aguirre<br>
    Melodic Spaceship<br>
    Jon "River Salmon" Sayer<br>
    Anthony Meade<br>
    Heath<br>
    Carlos "Cardboard Marty" Sanchez
</font>

</body>
</html>)");

	// required to display rich text
	wxHtmlWindow *htmlWin = new wxHtmlWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxHW_SCROLLBAR_AUTO);
	htmlWin->SetPage(credits);

	// Button to close the dialog
	wxButton *closeButton = new wxButton(this, wxID_OK, "Close");

	// Layout using a vertical box sizer for proper arrangement
	wxBoxSizer *vbox = new wxBoxSizer(wxVERTICAL);
	vbox->Add(htmlWin, 1, wxEXPAND | wxALL, 10);
	vbox->Add(closeButton, 0, wxALIGN_CENTER | wxALL, 10);

	SetSizer(vbox);
	Layout();

	Center(); // force everything to center
}
