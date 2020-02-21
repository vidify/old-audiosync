#!/usr/bin/env python3

# This benchmark will start playing songs on Spotify while running audiosync
# for each of them. The Spotify client must be open when this script is run
# for audiosync to record the output.
# 
# This benchmark is only useful to see the precision of the module, since the
# speed may depend on the internet connection and other factors.

import time
from tekore import util, Spotify
from tekore.util import prompt_for_user_token
from tekore.scope import scopes
import audiosync


# Lots of different songs. Some should be really easy to synchronize, while
# others have noise or long intros, and a couple of them are too repetitive.
songs = {
    '08mG3Y1vljYA6bvDt4Wqkj': 'AC/DC - Back In Black',
    '5EwwwdsQfKI8ZnFG93j5Zu': 'Martin Garrix - Forever',
    '4uLU6hMCjMI75M1A2tKUQC': 'Rick Astley - Never Gonna Give You Up',
    '0Y1FLqg7c4YFCKP2F6HXsG': 'Blood Red Shoes - It\'s Getting Boring By The Sea',
    '4qWCjHL1CtBV57uwdkhIXn': 'Mylky, KRAL & Refeci - Area 51',
    '1mea3bSkSGXuIRvnydlB5b': 'Coldplay - Viva La Vida',
    '2qC1sUo8xxRRqYsaYEdDuZ': 'Tycho - Awake',
    '21T9lWigjSIMg9uD6ZfRnQ': 'Killing Joke - Eighties',
    '25NCJC9DJ2FXJEz54wMHRp': 'Tom Grennan - Sober',
    '2JtzwKuk3RmxDSeoY6LMMI': 'kiss - love gun',
    '19503qDaxgCdPL2BhJngij': 'Maximo Park - Apply Some Pressure',
    '7FIWs0pqAYbP91WWM0vlTQ': 'Godzilla (feat. Juice WRLD) [Official Audio]',
    '51NFxnQvaosfDDutk0tams': 'Billie Eilish - Bellyache',
    '4VqPOruhp5EdPBeR92t6lQ': 'Muse - Uprising',
    '15V58YLX8jLoN7zlqPrObz': 'The Music - The People'
}

# Spotify authentication: you'll need SPOTIFY_CLIENT_ID, SPOTIFY_CLIENT_SECRET
# and SPOTIFY_REDIRECT_URI declared as environmental variables for this to
# work.
conf = util.config_from_environment()
scope = 'user-modify-playback-state'
token = util.prompt_for_user_token(*conf, scope=scope)
spotify = Spotify(token)

# Running audiosync
total = 0
for uri, name in songs.items():
    print(f">> Playing URI {uri} ({name})")
    start = time.time()

    spotify.playback_start_tracks([uri])
    audiosync.run(name)

    elapsed = time.time() - start
    total += elapsed
    print(f">> Took {elapsed} seconds")

print(f">> Total: {total} seconds")
