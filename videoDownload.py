
import os
import argparse
import subprocess
from yt_dlp import YoutubeDL
from yt_dlp.utils import DownloadError

def check_ffmpeg_installed():
    """Check if FFmpeg is installed and available in PATH."""
    try:
        subprocess.run(['ffmpeg', '-version'], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, check=True)
        return True
    except (subprocess.CalledProcessError, FileNotFoundError):
        return False

def download_youtube_video(url, output_path=None, resolution='best', audio_only=False):
    """
    Downloads a YouTube video or its audio using yt-dlp, with automatic conversion to MP4 for videos.

    Parameters:
    - url: str, the YouTube video URL
    - output_path: str, optional path to save the file (defaults to current directory)
    - resolution: str, video resolution (e.g., '720p', 'best' for best available)
    - audio_only: bool, if True, downloads only the audio

    Returns:
    - Path to the downloaded file (if successful)
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
            # Video download with MP4 preference and conversion
            format_str = 'bestvideo[height<={res}][ext=mp4]+bestaudio[ext=m4a]/bestvideo[height<={res}]+bestaudio/best[height<={res}]' if resolution != 'best' else 'bestvideo[ext=mp4]+bestaudio[ext=m4a]/bestvideo+bestaudio/best'
            # Convert resolution to int if possible (e.g., '720p' -> 720)
            res_num = resolution.rstrip('p') if resolution.endswith('p') else resolution
            format_str = format_str.format(res=res_num) if resolution != 'best' else format_str
            ydl_opts['format'] = format_str
            
            # Add postprocessor to convert to MP4 if necessary
            ydl_opts['postprocessors'] = [{
                'key': 'FFmpegVideoConvertor',
                'preferedformat': 'mp4',  # Convert to mp4 (remux if possible)
            }]
            
            print(f"Downloading video from: {url} at up to {resolution} (will be converted to MP4 if needed)")
        
        with YoutubeDL(ydl_opts) as ydl:
            info = ydl.extract_info(url, download=True)
            file_path = ydl.prepare_filename(info)
            # Adjust file_path for postprocessed files
            if audio_only:
                file_path = file_path.rsplit('.', 1)[0] + '.mp3'
            else:
                # For video, ensure it's .mp4 after conversion
                file_path = file_path.rsplit('.', 1)[0] + '.mp4'
            print(f"Download completed! File saved at: {file_path}")
            return file_path
    
    except DownloadError as e:
        print(f"Download error: {e}")
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
# 2. Install FFmpeg: On macOS, if you have Homebrew installed (brew --version to check), run 'brew install ffmpeg'. Otherwise, download from https://ffmpeg.org/download.html and add it to your PATH.
# 3. Usage example: python script.py "https://www.youtube.com/watch?v=example" -r 720p -o /path/to/save
# 4. Be aware that downloading YouTube videos may violate their Terms of Service. Use responsibly and for personal use only.
# 5. This script now prefers MP4 formats where available and uses FFmpeg to convert to MP4 if the downloaded format is WebM or similar.
# 6. For audio, it downloads as MP3. Adjust 'preferredquality' in the code if you want different bitrate.
