#ifndef ITEM_H
#define ITEM_H

#include <elc/core.h>

#undef _ITEMS_MACRO
#define _ITEMS_MACRO\
    X(NONE, 0, "", "", "")\
    X(ROCK, 64, "Rock", "a simple rock, used for many things", "\"When life puts you in tough situations, don't say 'Why me?' just say 'Try me!'\" - Dwayne Johnson")\
    X(THICK_PAPER, 64, "Thick Paper", "crude paper made from sugarcane, good for crafts, not so good for writing", "\"And scissors beat paper so lesbian sex will defeat fascism.\" - sushi_avocado")\
    X(SUGAR_CANE, 64, "Sugarcane", "a sweet tall grass plant used in the production of sugar and paper, it grows well in wet areas", "\"What is the fastest cane? Hurry-cane\" - impilcature")\
    X(WOODEN_STICK, 64, "Wooden Stick", "a small wooden branch, used for many things", "\"What's brown and sticky? A stick\" - Alex Hill")\
    X(WOODEN_LOG, 64, "Wooden Log", "a large chunk of a tree, can be split or cut into various things", "\"${jndi:ldap://203.0.113.0}\" - Random Skid")\
    X(IRON_INGOT, 64, "Iron Ingot", "a strong and common metal, mostly used for tools and buildings", "\"Had a big iron on his hip, big iron on his hip\" - Marty Robbins")\
    X(TRACTOR, 1, "Tractor", "a large vehicle, used mostly for pulling heavy equipment", "\"CUZ IM A BRISTOL CITY FAN AND I CAN DRIVE A TRACTOR\" - Eli M.")\
    X(COPPER_COIN, 64, "Copper Coin", "the least valuable galactic standard coin", "\"poor kids are just as bright and just as talented as white kids\" - Joseph Robinette Biden Jr.")\
    X(SILVER_COIN, 64, "Silver Coin", "a reasonably valuable galactic standard coin", "\"some flowers to keep out the uncultured middle class\" - kodiakwhale")\
    X(GOLD_COIN, 64, "Gold Coin", "a quite valuable galactic standard coin", "\"Got some money, got some cash, I'm rich, and now, I'm sad\" - Bill Wurtz")\
    X(PLATINUM_COIN, 64, "Platinum Coin", "the most valuable galactic standard coin", "\"None of us got where we are solely by pulling ourselves up by our bootstraps.\" - Thurgood Marshall")\
    X(GALACTIC_CREDIT, 64, "Galactic Credit", "it holds no intrinsic value, but is generally regarded to be worth ten platinum coins", "\"These parasites had it coming\" - Luigi Nicholas Mangione")\
    X(GIANT_TOOTH, 16, "Giant Tooth", "completely and utterly useless, found in ancient dental office ruins", "\"This video is just a painful reminder that England exists\" - Milo Rossi")\
    X(GOLD_PISTOL, 1, "Golden Pistol", "a pistol of gold with an almost magic scratchless shine", "\"I shall never be happy until everything that I touch becomes gold\" - King Midas")\
    X(DAMAGE_SCROLL, 64, "Scroll of Damage", "a paper scroll with horrific symbols written in blood", "\"Do you fear death? Do you fear that dark abyss? All your deeds laid bare, all your sins punished?\" - Davy Jones")\
    X(COMMON_MECH_PART, 64, "Common Mechanical Part", "a little thingymajig used in building basic robots and other machines", "\"A robot may not injure a human being or, through inaction, allow a human being to come to harm.\" - Isaac Asimov")\
    X(RARE_MECH_PART, 64, "Rare Mechanical Part", "a rather complex thingymajig used in building robots and other machines", "\"A robot must obey the orders given it by human beings except where such orders would conflict with the First Law.\" - Isaac Asimov")\
    X(MYTHICAL_MECH_PART, 64, "Mythical Mechanical Part", "a very complex gadget used in building advanced robots and other machines", "\"A robot must protect its own existence as long as such protection does not conflict with the First or Second Law.\" - Isaac Asimov")\
    X(LEGENDARY_MECH_PART, 64, "Legendary Mechanical Part", "a very complex gadget used in building advanced robots and other machines", "\"Never let your sense of morals keep you from doing what is right.\" - Isaac Asimov")\
    X(INFINITE_STORAGE, 1, "Infinite Storage Device", "a gizmo covered box covered that looks much bigger on the inside", "\"Placeholder quote\" - John Placeholder")\
    X(PORTABLE_PORTAL, 2, "Porta Portal", "a clear glass sphere filled with wires and tubes and a red button on one side", "\"Placeholder quote\" - John Placeholder")\
    X(WOBBLE_PANCAKE, 64, "Wobbledog Pancake", "a brightly colored sweet smelling pancake, it looks like edible plastic", "\"Placeholder quote\" - John Placeholder")\
    X(GOLD_BRAIN, 64, "Golden Brain", "a shiny gold yet still goopy brain, ocasionally found in the skull of a zombie", "\"Placeholder quote\" - John Placeholder")\
    X(EXPERIENCE_LINK, 1, "Experience Link", "two gray and blue bracelets connected by a bundle of wires, transfers half of one wearers' experience to the other", "\"Placeholder quote\" - John Placeholder")\
    X(MANA_CORE, 1, "Mana Core", "a small half glass half metal sphere, the first half housing a fluid tank and half a jumble of wires, used to cast spells without a living creature", "\"Placeholder quote\" - John Placeholder")\
    X(DAMAGE_RUNE, 1, "Rune of Damage", "a dark red gemstone with menacing glyphs similar to those on the Scroll of Damage etched into it", "\"Placeholder quote\" - John Placeholder")\
    X(TIMEKEEPER_CLOCK, 1, "The Timekeeper's Clock", "an extremely powerful object, taken from the god of time himself", "\"Placeholder quote\" - John Placeholder")\
    X(MATERIALIZATION_STONE, 64, "Stone of Materialization", "a round rainbow gemstone with a glow inside constantly changing shape into various items", "\"Placeholder quote\" - John Placeholder")\
    X(MATERIALIZATION_SHARD, 64, "Shard of Materialization", "a small shard chipped off from a Stone of Materialization, it still has a faint morphing glow inside", "\"Placeholder quote\" - John Placeholder")\
    X(UNIVERSE_CONSOLE, 1, "Universe Console", "a yellowed CRT display and mechanical keyboard with a cable coming from the display and ending in a glowing blue rift", "\"Placeholder quote\" - John Placeholder")\
    X(ZOMBIE_BRAIN, 64, "Zombie Brain", "a goopy decomposing brain taken from a zombie, technically edible but otherwise useless", "\"Placeholder quote\" - John Placeholder")\
    X(SOULS_JAR, 64, "Jar of Souls", "a jar filled with silently screaming ghostlike figures, it seems they desperately crave freedom", "\"Placeholder quote\" - John Placeholder")\
    X(DRAGON_SCALE, 64, "Dragon Scale", "an extremely tough and rigid scale shed by a dragon, it smolders with bits of remaining fire", "\"Placeholder quote\" - John Placeholder")

typedef u8 ItemType;

typedef struct ItemTypeInfo {
    const char* name;
    const char* description;
    const char* quote;
    u8 max_stack;
} ItemTypeInfo;

#undef X
#define X(id, max_stack, name, description, quote) ITEM_TYPE_##id,
enum {
    _ITEMS_MACRO
};
#undef X
#define X(id, max_stack, name, description, quote) case ITEM_TYPE_##id: return (ItemTypeInfo){name, description, quote, max_stack};
ELC_INLINE ItemTypeInfo getItemTypeInfo(ItemType type) {
    switch (type) {
        _ITEMS_MACRO
    }
    return (ItemTypeInfo){0};
}
#undef X
#undef _ITEMS_MACRO

#endif
