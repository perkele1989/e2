

    Reveal and Annihilate
    Concept 2 

    Grand Strategy Tower Defense 
    Turn-based

    Plot:

        You are a technologically advanced WW2-era german civilization, in a timeline where Germany won the war.

        During a research experiment, the Finnish city of Mikkeli got transferred to an alternate dimension.

        In this dimension, humanity is much less evolved, and consist mostly of angry but strong cavemen.

        As they explore this dimension, they realize that they are not the only intelligent lifeform on the planet,

        and weird meteorites found on the surface of the earth can be hatched by sledge-wielding engineers.


    Rules:

        You start with a scout unit, that can be moved to a tile of the players choosing, and then deployed into

        a city. The first city founded is Mikkeli, and other cities are named after other Finnish cities in proximity
        
        to Mikkeli.

        Once a city is founded, you can build engineers and other scouts from this city. Engineers can improve tiles
        
        to generate resources (economy), as well as build city improvements in adjacent city tiles, and defensive towers.

        Engineers can also be used to trigger "hives" (meteorites or primitive settlements), that in turn will trigger

        tower-defense style waves.

        When in a wave, enemy units will spawn from the hive, and path-find themselves towards the closest city tiles, 

        where they will deal damage by simply colliding with the main city tile, and then perish.

        The objective of the game is for the player to build defensive towers to create mazes for, and attack the enemy

        units.

        Once all cities are destroyed, the player loses, and the lose condition is met.

        Killing enemy units is the only way to generate gold, which can then be used to expand your base.

        The game continues on infinitely, both temporally but also spatially, as long as the player does not lose.

        Hives are continuously spawned in a balanced proximity to cities, but only a certain amount can exist at the same
        
        time.

    Resources:

        Gold: Yieldable by killing enemy units in waves. Used to build and upgrade units and structures.

        Wood: Yieldable by building a Saw Mill on forested tiles. Used to build certain units and structures, or to yield steel.

        Iron: Yieldable by building a Refinery on iron ore tiles. Used to build certain units and structures, or to yield steel.

        Steel: Yieldable by combining wood and iron (needs Steel Mill city improvement). Used to upgrade units and structures (needs Research Facility).

        Productivity: Every structure gives productivity, and can be increased via upgrades. Productivity does not sum up like the other resources, but is a constant net factor.
                      High productivity scales time it takes to build units and research upgrades to be faster
                      i.e. 16 productivity might translate to 0.7x build time, 32 might be 0.45x etc.

                      Productivity is on a per-city basis

    Entities:

        Any entity upgrades requires them to be researched, and then purchased per-entity using Steel 

        City
            Train Engineer (Cost 3 gold, 3 turns)
            Train Scout (Cost 5 gold, 20 turns)

        Saw Mill
            Generates Wood per turn (+100% if adjacent to water tile)

        Refinery
            Generates Iron per turn (+50% for each adjacent untouched forest tile)

        Steel Mill (City improvement)
            Yields Steel per turn (+100% if adjacent to water tile)
            Yields +2 productivity (+4 additional for each adjacent mountain tile)

        Research Facility (City Improvement)
            Research unit and building upgrades (costs Turns)

        Industrial Zone  (City Improvement)
            Yields +10 productivity, (+10 additional if adjacent to Steel Mill, +5 additional if adjacent to refinery, +5 additional if adjacent to sawmill)

        Scout
            Found City (unit perishes)

        Engineer 
            Upgrade Diligence (2 Gold), one extra build action per turn
            Poke Hive (unit perishes), triggers 5 or 10 waves from given hive and then hive persishes
                Only available if hive can find path to city

            Build Saw Mill (2 Gold, 2 Wood)
            Build Refinery (5 Gold, 10 Wood)
            Build Research Facility (20 Gold, 20 Steel)
            Build Steel Mill (20 Gold, 5 Wood, 5 Iron requires Saw Mill and Refinery in same city)

            Build Gatling Tower (5 Gold, 5 Iron, 2 Wood)
            Build Mortar Tower (5 Gold, 5 Iron, 5 Wood)
            Build Flamethrower Tower (10 Gold, 10 Steel)
            Build Observation Tower (10 Gold, 10 Wood)

        Gatling Tower 
            Pewpew's enemy units

            Upgrade Steel Barrels       I, II, III (2, 4, 8 Steel), higher damage, longer range
            Upgrade Brushless Motors    I, II, III (2, 4, 8 Steel), faster cyclic rate 
            Upgrade Lidar Tracking       I, II, III (2, 4, 8 Steel), faster unit tracking, higher accuracy

        Mortar Tower 
            Fompfomp's enemy units

            Upgrade Smokeless Powder    I, II, III (2, 4, 8 Steel), higher damage, longer range 
            Upgrade Autoloader          I, II, III (2, 4, 8 Steel), faster cyclic rate
            Upgrade Cluster Grenades    I, II, III (4, 8, 12 Steel), more grenades per grenades fired

        Flamethrower Tower 
            Zizzle's enemy units

            Upgrade Diesel Fuel         I, II, III (4, 8, 12 Steel), higher damage, longer burn time (blue fire)
            Upgrade Nitrogen Gas        I, II, III (4, 8, 12 Steel), longer range
            Upgrade Napalm              I, II, III (4, 8, 12 Steel), applies burning status effect (1, 3, 7 seconds)

        Observation Tower 
            Increases attack range for units within affective range 
            High visibility range, further increasing effective range (since towers cant attack units in fog of war)

            Upgrade Radar Technology I, II, III, upgrades attack range by 1, 2, 4
            Upgrade Communications Technology I, II, III, upgrades affective range by 1, 2, 4



    Research:
        Diligence (10 turns), increases productivity on all structures by 2x
        Radar Technology I, II, III (3, 5, 10 turns)
        Communications Technology I, II, III (3, 5, 10 turns)
        Steel Barrels I, II, III (3, 5, 10 turns)
        Brushless Motors I, II, III (3, 5, 10 turns)
        Lidar Tracking I, II, III (3, 5, 10 turns)
        Smokeless Powder I, II, III (3, 5, 10 turns)
        Autoloader I, II, III (3, 5, 10 turns)
        Cluster Grenades I, II, III (5, 10, 15 turns)
        Diesel Fuel I, II, III (3, 5, 10 turns)
        Nitrogen Gas I, II, III (3, 5, 10 turns)
        Napalm I, II, III (5, 10, 15 turns)


    UX:

        Resources:
            Gold, sum and per turn
            Wood, sum and per turn
            Iron, sum and per turn
            Steel, sum and per turn 


        Overlays:
            Resources (shows resource icons)
            Maze view (shows enemy obstacles)


    Assets
        Meshes 
            City (Town -> City)
            Steel Mill
            Research Facility
            Industrial Zone

            Saw Mill
            Ore Resource (unimproved refinery)
            Refinery
            

            Gatling Tower
            Mortar Tower
            Flamethrower Tower
            Observation Tower

            Scout Unit
                Idle 
                Run
                Found City

            Engineer Unit 
                Idle
                Run
                Build/Poke Hive (Just smash ground with sledge)

            Hives???