import os
import argparse
import subprocess
import shutil
from yt_dlp import YoutubeDL
from yt_dlp.utils import DownloadError

def check_ffmpeg_installed():
    """Check if FFmpeg and ffprobe are installed and available in PATH."""
    return shutil.which('ffmpeg') is not None and shutil.which('ffprobe') is not None

def ffprobe_codec(path: str, stream_selector: str):
    """
    Returns codec name for the selected stream (e.g., 'v:0' or 'a:0'), or None if missing.
    """
    try:
        out = subprocess.check_output(
            [
                "ffprobe", "-v", "error",
                "-select_streams", stream_selector,
                "-show_entries", "stream=codec_name",
                "-of", "default=nw=1:nk=1",
                path,
            ],
            stderr=subprocess.STDOUT,
        ).decode().strip()
        return out or None
    except subprocess.CalledProcessError:
        return None

def webm_to_mp4(input_path: str, output_path: str, crf: int = 18, preset: str = "slow"):
    """
    Convert a .webm to an HD .mp4. Remux if already compatible; otherwise re-encode to H.264/AAC.

    - crf: 18-22 is a good range (lower = higher quality)
    - preset: ultrafast..veryslow (slower = more efficient compression)
    """
    if not check_ffmpeg_installed():
        raise RuntimeError("ffmpeg/ffprobe not found in PATH. Please install FFmpeg.")

    if not os.path.exists(input_path):
        raise FileNotFoundError(f"Input not found: {input_path}")

    vcodec = ffprobe_codec(input_path, "v:0")
    acodec = ffprobe_codec(input_path, "a:0")

    if vcodec is None:
        raise ValueError("No video stream found in the input file.")

    # Attempt remux if codecs are already MP4-friendly
    if vcodec in {"h264", "mpeg4"} and acodec in {"aac", "mp3"}:
        cmd = [
            "ffmpeg", "-y", "-i", input_path,
            "-c", "copy",
            "-movflags", "+faststart",
            output_path,
        ]
    else:
        # Re-encode to QuickTime-friendly H.264/AAC, preserving sharpness
        cmd = [
            "ffmpeg", "-y", "-i", input_path,
            "-map", "0:v:0?", "-map", "0:a:0?",
            "-c:v", "libx264", "-preset", preset, "-crf", str(crf),
            "-profile:v", "high", "-level", "4.1", "-pix_fmt", "yuv420p",
            "-c:a", "aac", "-b:a", "192k",
            "-movflags", "+faststart",
            output_path,
        ]

    print("Running conversion:", " ".join(cmd))
    subprocess.run(cmd, check=True)
    print(f"Conversion completed! MP4 saved at: {output_path}")

def download_youtube_video(url, output_path=None, resolution='best', audio_only=False):
    """
    Downloads a YouTube video or its audio using yt-dlp, then converts WebM -> MP4 with FFmpeg.

    Parameters:
    - url: str, the YouTube video URL
    - output_path: str, optional path to save the file (defaults to current directory)
    - resolution: str, video resolution (e.g., '720p', 'best' for best available)
    - audio_only: bool, if True, downloads only the audio

    Returns:
    - Path(s) to the downloaded file(s)
    """
    if not check_ffmpeg_installed():
        raise EnvironmentError("FFmpeg is not installed or not found in PATH. Please install FFmpeg to merge audio/video streams and convert formats. On macOS, use 'brew install ffmpeg' if you have Homebrew.")

    try:
        # Set output path
        if output_path is None:
            output_path = os.getcwd()
        output_template = os.path.join(output_path, '%(title)s.%(ext)s')

        # Common options
        ydl_opts = {
            'outtmpl': output_template,
            'quiet': False,
            'no_warnings': False,
        }

        if audio_only:
            # Audio-only download (MP3 format)
            ydl_opts.update({
                'format': 'bestaudio/best',
                'postprocessors': [{
                    'key': 'FFmpegExtractAudio',
                    'preferredcodec': 'mp3',
                    'preferredquality': '192',
                }],
            })
            print(f"Downloading audio from: {url}")
        else:
            # Video download:
            # Prefer sharper VP9 video; cap by resolution if provided, else best
            if resolution == 'best':
                format_str = 'bestvideo[vcodec^=vp9]+bestaudio/bestvideo+bestaudio/best'
            else:
                res_num = resolution.rstrip('p') if isinstance(resolution, str) and resolution.endswith('p') else str(resolution)
                format_str = f'bestvideo[height<={res_num}][vcodec^=vp9]+bestaudio/bestvideo[height<={res_num}]+bestaudio/best[height<={res_num}]'

            ydl_opts['format'] = format_str
            ydl_opts['merge_output_format'] = 'webm'  # Merge into WebM to preserve VP9 quality

            # Remove previous MP4 postprocessor — we’ll do our own conversion
            # ydl_opts['postprocessors'] = [...]  # Not used

            print(f"Downloading video from: {url} at up to {resolution}")
            print(f"Format selector: {format_str}")
            print("Merging into WebM first for quality; will convert to MP4 after download")

        with YoutubeDL(ydl_opts) as ydl:
            info = ydl.extract_info(url, download=True)

            # Determine final downloaded path
            requested = [d.get('filepath') for d in (info.get('requested_downloads') or []) if d.get('filepath')]
            downloaded_path = requested[-1] if requested else ydl.prepare_filename(info)

            if audio_only:
                mp3_path = os.path.splitext(downloaded_path)[0] + '.mp3'
                print(f"Download completed! File saved at: {mp3_path}")
                return mp3_path
            else:
                # Ensure WebM path exists
                webm_path = downloaded_path if downloaded_path.endswith('.webm') else os.path.splitext(downloaded_path)[0] + '.webm'
                if not os.path.exists(webm_path):
                    # Some formats may end up mp4; handle that gracefully
                    alt_mp4 = os.path.splitext(downloaded_path)[0] + '.mp4'
                    if os.path.exists(alt_mp4):
                        print(f"Download completed as MP4 (no conversion needed): {alt_mp4}")
                        return alt_mp4
                    raise FileNotFoundError(f"Expected merged WebM not found: {webm_path}")

                print(f"Download completed! WebM file saved at: {webm_path}")

                # Convert WebM -> MP4 using robust converter
                mp4_path = os.path.splitext(webm_path)[0] + '.mp4'
                webm_to_mp4(webm_path, mp4_path, crf=18, preset="slow")

                print(f"Both files available:\n- WebM: {webm_path}\n- MP4:  {mp4_path}")
                return webm_path, mp4_path

    except DownloadError as e:
        print(f"Download error: {e}")
    except subprocess.CalledProcessError as e:
        print("FFmpeg failed during conversion.")
        raise
    except Exception as e:
        print(f"An unexpected error occurred: {e}")

def main():
    parser = argparse.ArgumentParser(description="YouTube Video Downloader using yt-dlp")
    parser.add_argument("url", help="YouTube video URL")
    parser.add_argument("-o", "--output", help="Output directory (default: current directory)")
    parser.add_argument("-r", "--resolution", default="best", help="Video resolution (e.g., '720p', default: best)")
    parser.add_argument("-a", "--audio", action="store_true", help="Download audio only (as MP3)")

    args = parser.parse_args()

    download_youtube_video(args.url, args.output, args.resolution, args.audio)

if __name__ == "__main__":
    main()

# Important Notes:
# 1. Install yt-dlp: pip install yt-dlp
# 2. Install FFmpeg (and ffprobe): On macOS, 'brew install ffmpeg'; Windows: use gyan.dev builds; Linux: package manager.
# 3. Usage example:
#    python script.py "https://www.youtube.com/watch?v=example" -r 1080p -o /path/to/save
#    (Tip: for args use -r best or -r 1080p; do not write -r -1080p)
# 4. This version downloads high-quality VP9 streams (sharper), merges to WebM, then converts to QuickTime-friendly MP4 using H.264/AAC.
# 5. For even higher quality, try crf=16 and preset="veryslow" in webm_to_mp4 (slower, larger).
# 6. If the download ends up already in MP4, the script skips conversion and returns the MP4 path.