
* [日本語](/readme.md)

# What is Hengband?

A dungeon-crawler where you descend down a dungeon fighting hordes of monsters, getting stronger and trying to stay alive.

# How to install

## Windows

 * Download the latest binaries from [Releases](https://github.com/hengband/hengband/releases) - look for a `.zip` archive.
 * Unpack the archive in the directory of your choosing.
 * Double click on `Hengband.exe` to start the game.

**Note:** The first time you try Windows Smartscreen may ask you if you want to run this software. To bypass it you need
to first click on the text link *More info* and then the button *Run anyway*

### Menu bar:

Some elements of the interface and game can be customized using the menu bar:

 * **Sound Effects:** Options -> Sound
 * **Music:** Options -> Music
 * **Background image:** This will display an image in the background of the game.
   * Options -> Background image -> None - disable the background
   * Options -> Background image -> Custom - image selected by you (on first select will prompt you to select a file)
   * Options -> Background image -> Change custom image - changes the selected Custom image

## UNIX

Download the latest version of the source files from [Releases](https://github.com/hengband/hengband/releases). Then run the following commands:

```
tar -jxvf hengband-x.x.x.tar.bz2
cd hengband-x.x.x
./configure --disable-japanese
make install
```

**Please note:**

 * Replace `x.x.x` with the downloaded version's number.
 * `--disable-japanese` is required to build English version, remove it if you want to build Japanese version.

To run in ASCII mode:

```
./hengband -- -n <number of windows>
```

To run in the graphical mode:

```
./hengband -g -- -n <number of windows> # 8x8 Tiles
./hengband -g -- -b -n <number of windows> ## double width size tiles
./hengband -g -- -a -n <number of windows> ## 16x16 tiles (tiles of this size are not provided)
```

### Install with `setgid`

Run the following:

```
./configure --disable-japanese --with-setgid=games
make
```

Running the following as root or with the sudo command will install the game into `/usr/local`:

```
make install
```

See `./configure --help` for more information about changing the installation location and other options.

### Configuring display


The default font used in the main window is:

>  "-*-*-medium-r-normal--24-*-*-*-*-*-iso8859-1,-*-*-medium-r-normal--24-*-*-*-*-*-jisx0208.1983-0"

The default font used in the sub windows is:

> "-*-*-medium-r-normal--16-*-*-*-*-*-iso8859-1,-*-*-medium-r-normal--16-*-*-*-*-*-jisx0208.1983-0"

These can be controlled by setting environment variables:

 * `ANGBAND_X11_FONT_<?>` - Sets the font for a specific window, from 0 to 7, eg `ANGBAND_X11_FONT_0`
 * `ANGBAND_X11_FONT` - Sets the font for all the remaining windows

You can also change other window properties:

 * `ANGBAND_X11_AT_X_<?>` - X position of a window
 * `ANGBAND_X11_AT_Y_<?>` - Y position of a window
 * `ANGBAND_X11_COLS_<?>` - Width of the window in *half-characters*
 * `ANGBAND_X11_ROWS_<?>` - Height of the window in *half-characters*

### Other notes

When using **XIM** with **XFree86 4.0** you need to set the environment variable `XMODIFIERS` to either `@im=skkinput` or `@im=kinput2`.


# Getting started

Hengband is a roguelike game that has its roots in Moria, originally released in 1988. Across hundreds of updates
and many variants the game grew in complexity. At first playing it might seem complicated, confusing and impenetrable,
especially with no fancy graphics, but the basics are very easy to learn.

## Character creation

Once you start Hengband you'll want to use File -> New in the menu and create your first character.
For the start we recommend:

 * **Sex:** any
 * **Race:** dwarf, so that you can survive more
 * **Class:** warrior, as it's simple to play
 * **Personality:** ordinary, to keep things simple
 * **Birth options:** do not modify anything, press `Escape`
 * **Autoroller stats:** do not change anything, use arrows to select *Accept* and press `Enter`
 * **Rolling stats:** keep hitting `r` to reroll your stats, *s* to toggle between last two rolls,
  *Enter* once you're happy with your stats. It's a good idea to reroll a few times until you get
  good amount of Hit points is higher than an average roll
 * **Name:** give your character a name you'll like staring at.
 * **Character background:** you can edit the character background or leave it as-is (it has no
  gameplay effect). Press `Enter` when ready.
 * **Summary:** At this point you can either hit `S` to start over, `Q` to quit or `Enter` to begin the game.

## How to move and act

 * Use numeric keyboard to move your character in eight directions, and press 5 to wait a turn.
 * Walk into enemies to attack them with your melee weapon.
 * Attacking and killing enemies will give you experience, which in turn lets you level up and get stronger.
 * Walk up/down the stairs using `<` and `>`.

## What to do first

 * A good first step is to exchange Wooden Torches you start with to Brass Lanterns:
   * Step into the General Store (step on the brown number 1, it's the entrance).
   * Hit `s` and sell the Torches.
   * Hit `b` to buy and chose Brass Lanterns.
   * When asked for quantity type `1`.
   * Hit `Enter` to quickly accept the price and buy the lantern.
   * Hit `Escape` to leave the shop.
   * Hit `w` to wear and select the purchased lantern.
 * It might also be a good idea to buy one or two Flasks of Oil from the same shop. Use command `F` to fuel
   the lantern.
 * Next it's advised you go slightly East of the entrance to the town into Yeek Cave. Step on purple `>`
   and press that same key on your keyboard to enter the dungeon.
 * Explore, kill monsters, collect items, go back to town to sell those items and buy better equipment.
   The world is yours to explore!

## Good to know

 * When in town you can leave it to enter the wilderness. You can also enter the overworld map by pressing `<`, though
   keep in mind you can be ambushed and you'll use a lot of food there.
 * If you die you die forever. Next time you'll need to either restart with the same character or a create a new one.
   If you don't like playing that way, there are two ways to work around this:
   * Open options `=` -> Gameplay Options then toggle *Allow use of debug/cheat options*, then go back to list of options
     -> Cheat Options and toggle *Allow player to avoid death*
   * Manually backup saves under `lib/save/` and restore the backups when you die.
   * Permadeath is seen as part of the fun as it greatly raises the stakes and causes you to play differently. But it
     can be very frustrating too, especially when you're still learning.

## Controls

At any point in game you can press `?` to open the help. A grouped list of actions can be displayed by pressing `Enter`.

 * **[Movement]**
   * `Numeric Keyboard` - Move around
   * `Numeric 5` - Stay still
   * `Shift+direction` - Run
   * `Ctrl+direction` - Attack/Open/Close/Dig in that direction
   * `<` - Go up stairs/Enter the overworld map
   * `>` - Go down stairs/Leave the overworld map
 * **[Tools]**
   * `a` - Aim a wand
   * `A` - Activate special power of an equipped item
   * `E` - Eat food
   * `f` - Fire a missile
   * `F` - Fuel your torch/lantern
   * `r` - Read a scroll
   * `u` - Use a staff
   * `q` - Quaff a potion
   * `v` - Throw an item
   * `z` - Zap a rod
 * **[Actions]**
   * `d` - Drop an item
   * `g` - Pick up an item (You can also enable Auto Pickup in Options -> Input Options -> Pick things up by default)
   * `l` - Destroy an item
   * `o` - Open a door or chest
   * `R` - Rest for a period (The default option `&` rests until you are fully rested)
   * `s` - Search for traps/doors around you
   * `S` - Toggle Search mode, where you act slower but keep looking on each move
 * **[Magic]/[Abilities]**
   * `b` - Examine spells/prayers/mental powers
   * `G` - Gain new spells/prayers/mental powers
   * `m` - Cast a spell/use a mental power
   * `U` - Use a special power
 * **[Equipment]**
   * `t` - Take off an equipped item
   * `w` - Wear/wield an item
 * **[Info]**
   * `C` - Character information
   * `e` - Display equipped items
   * `i` - Display inventory
   * `I` - Displays information about an \*identified* item
   * `l/*` - Inspect tiles
   * `L` - Inspect the map around you
   * `M` - Show full dungeon map
   * `~` - Display various information
 * **[Other]**
   * `@` - Setup macro and keymaps
   * `=` - Change options
   * `$` - Reload auto-picker preferences
   * `_` - Edit auto-picker preferences (Auto-picker only works when *Pick things up by default* is disabled)
   * `n` - Repeat previous command

## Item notation

* `(XdY)`
  * Notation used for melee weapons, indicates the base damage dice of the weapon.
  * The total damage is X rolls of Y-sided dice. For example:
    * `1d12` is damage range of 1-12, average 6.5 damage
    * `2d6` is damage range of 2-12, average 7 damage
    * `3d4` is damage range of 3-12, average 7.5 damage
* `(+h,+d)`
  * Usually used in melee weapons, but can sometimes appear on other equipment as well.
  * `+h` indicates the bonus to accuracy of a melee attack.
  * `+d` indicates the bonus to damage of a melee attack.
  * Either parameter can be negative.
  * Some heavy armor will omit `+d` and only display something like `(-2)` indicating a negative bonus to accuracy.
* `[a,+b]` or `[+b]`
  * Used on armor.
  * `a` indicates the natural increase to AC of the piece of armor.
  * `+b` indicates the magical bonus.
* `(+p)` or `(+p Noun)`
  * Indicates a modifier to item's unique effect.
  * For example, in case of a *Ring of Damage* it indicate the bonus to damage.
  * `Noun` is used in weapons and armors, telling what the bonus is for.
* `(xN)`
  * Notation used in projectile weapons.
  * `N` is the damage multiplier for projectiles.

### Example 1: Dagger (2d4) (+10,+15) [+2] (+3)

 * `(2d4)` - Base damage dice
 * `(+10,+15)` - +10 accuracy bonus, +15 damage bonus
 * `[+2]` - Magically increases AC by +2
 * `(+3)` - Power of the special ability of this dagger, which can be known after \*Identifying* (or researching) it.

### Example 2: Elven Chainmail (-2) [14,+12] (+3 Stealth)

 * `(-2)` - -2 accuracy bonus
 * `[14,+12]` - 14 base AC bonus, +12 magical AC bonus
 * `(+3 Stealth)` - Increases Stealth stat by 3


# Advanced play

## Macros

You will often find yourself repeating the same combination of keypresses multiple times during your game, things like
shooting at the closest target with your bow or casting a bunch of buffs on yourself at once. Macros allow you to
do that with one keypress.

* Press `@` to open *Interact with Macros* page.
* You have two options here:
  * **Macros** are a number of keys pressed in sequence, that will trigger some action. For example setting it to `lpo`
    would require you to press keys `l`, `p` and `o` in quick succession to trigger the effect.
  * **Keymaps** define a new behavior for a single keypress. It's advised to use keys which normally have no mapping (
    the easiest way to find those is to just keep pressing keys on your keyboard and see which one shows gibberish).
* Now to create your new macro/keymap:
  * Press the key for either *Create a macro* or *Create a keymap*
  * Press the key sequence/key you want to assign a new action
  * Type the sequence of actions you want to trigger

## Inscriptions

Items can be inscribed to add notes but also to trigger special behavior. Below is a list of special inscriptions:

 * `=g`
    * Will auto-pick these items from the floor.
    * Most convenient on ammunition or items you throw.
    * Takes priority over Auto-Picker - even if it'd normally destroy the object, with this inscription the item will
      be picked up
 * `#<NAME>`
    * Displays the name as if it was an artifact name of the item.
    * For example: **a Broad Sword (2d5) (+0,+0)** inscribed with `#of Yeeks` will be displayed as **a Broad Sword of Yeeks (2d5) (+0,+0)**
 * `@X?`
    * Allows you to use a shortcut to access the item with a given command regardless of the position of the item in your inventory.
    * `X` is the command, eg `f`
    * `?` is a character you'll type to access the item. Can be a number or a letter.
    * For example: Tagging arrows with `@f1` will allow you to add a macro `f1*t` to Fire, select those arrows, target closest, fire.
 * `@?`
    * Similar to the above, but affects all commands.
    * `?` must be a number
 * `!X`
    * Prevents accidentally using the item with the specified command.
    * Trying to use the item with specified command will ask you if you really want to do that.
    * If you mark it with `!*` it will work for all commands.
 * `.`
    * Will prevent "Do you want to teleport?" confirmation from appearing if triggered by the inscribed item.
 * `$`
    * Disables warnings generated by the inscribed item.
 * `^X`
    * Similar to `!X` but affects equipped items.
 * `%%` or `%%all`
    * Inscribing an item with these will replace the inscription with a list of resistances and abilities of the
      inscribed weapon or armor.
    * It will only display known resistances and abilities - an item has to be either \*Identified* or researched.
    * `%%` will only display resistances and abilities which are not directly known from item's ego type.

You can combine multiple inscriptions. For example `@w0@t1!k!k!d#of Corwin` will:

 * Use `0` with command `w` to equip the item
 * Use `1` with command `t` to take off the item
 * Require two confirmations if you try to destroy it
 * Require a confirmation if you try to drop it
 * Will display the item as if it was named *of Corwin*
