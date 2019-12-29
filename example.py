import sys
import audiosync
import youtube_dl


# Checking that the script was used correctly
if len(sys.argv) != 2:
    print(f"Usage: {sys.argv[0]} \"SONG_NAME\"\n"
          "After running audiosync.get_lag, start playing the song in the"
          " background.")
    exit()

# Obtaining an URL from a song name with youtube_dl
ytdl_opts = {
    'format': 'bestaudio'
}
with youtube_dl.YoutubeDL(ytdl_opts) as ytdl:
    info = ytdl.extract_info(f"ytsearch:{sys.argv[1]}", download=False)
url = info['entries'][0]['url']

# After this is printed, the music should start playing in the background too
print("Running audiosync.get_lag")
ret = audiosync.get_lag(url)
print(f"Returned value: {ret}")
