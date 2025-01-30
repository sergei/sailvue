import argparse
import hashlib
import re
import os
import shutil
from datetime import datetime, timedelta

import requests
from mutagen.mp3 import MP3

CHUNK_SIZE = 1024

VOICE_IDS = {
    'Paul': 'WLKp2jV6nrS8aMkPPDRO',
    'Alexander': 'bWD9lIQeeSXBIWPT0mu4',
    'Jacob': 'KHx6YfZBu23HH6GJtSrW',
    'CS': 'tWGXkYJGea4wMBN4mLD1',
    'Pirate': 'PPzYpIqttlTYA83688JI',
    'Joe': 'PPzYpIqttlTYA83688JI',
    'Shirley': 'L4so9SudEsIYzE9j4qlR',
}


def get_voice_id(require_voice_name, text, default_voice_id):
    speaker = re.search(r'\[(.*?)]:', text)
    if speaker:
        speaker = speaker.group(1)
        print(f'Speaker: {speaker}')
        if speaker in VOICE_IDS:
            default_voice_id = VOICE_IDS[speaker]
        else:
            print(f'Unknown speaker: {speaker}')
        # Remove speaker name from the text along with the leading and training spaces
        text = re.sub(r'\[(.*?)\]:', '', text).strip()
    else:
        if require_voice_name:
            print('No speaker found in the text')
            return text, None

    return text, default_voice_id, speaker


def make_voiceover_file(number, text, api_key, voice_id, output_dir):
    # Make text hash to check if we already have the voiceover
    text_hash = hashlib.md5(text.encode()).hexdigest()
    # Construct file name from first few words and a hash
    file_name = text.split()[:4]
    file_name = '_'.join(file_name)
    # Remove any special characters from the file name
    file_name = re.sub(r'\W+', '', file_name)

    file_name += '-' + text_hash
    file_name = output_dir + os.sep + f'{number:03d}-' + file_name + '.mp3'

    # Check if MP3 file with the same hash already exists in the output directory
    files = os.listdir(output_dir)
    for file in files:
        if file.lower().endswith('.mp3'):
            t = file.split('-')
            if len(t) > 1 and t[2] == text_hash + '.mp3':
                current_name = output_dir + os.sep + file
                print(f'File {current_name} for this text already exists, skipping ...')
                return current_name

    # Make the voiceover using eleven labs API
    url = f"https://api.elevenlabs.io/v1/text-to-speech/{voice_id}"
    headers = {
        "Accept": "audio/mpeg",
        "Content-Type": "application/json",
        "xi-api-key": api_key
    }

    data = {
        "text": text,
        "model_id": "eleven_multilingual_v2",
        "voice_settings": {
            "stability": 0.6,
            "similarity_boost": 0.6,
            "style": 0.5,
            "use_speaker_boost": True,
        }
    }

    print(f'Creating file {file_name} ...')
    response = requests.post(url, json=data, headers=headers)
    # Check if the request was successful
    if response.status_code != 200:
        print(f'Error making voiceover: {response.text}')
        return None

    with open(file_name, 'wb') as f:
        for chunk in response.iter_content(chunk_size=CHUNK_SIZE):
            if chunk:
                f.write(chunk)

    print(f'File name: {file_name} created')
    return file_name


def decode_time(line):
    """ Decodes SRT time stamp such as 00:03:20,476 --> 00:03:22,671
    """
    idx = line.find('-->')
    s = line[0:idx].strip()
    time_from = datetime.strptime(s, "%H:%M:%S,%f")
    s = line[idx+3:].strip()
    time_to = datetime.strptime(s, "%H:%M:%S,%f")
    return time_from, time_to


def make_chapter(number, start_time, end_time, text, file_name, speaker):
    audio = MP3(file_name)
    mp3_duration_sec = timedelta(seconds=audio.info.length)
    text_hash = os.path.splitext(os.path.basename(file_name))[0].split('-')[2]
    return {
        'number': number,
        'start_time': start_time,
        'end_time': end_time,
        'text': text,
        'mp3_duration_sec': mp3_duration_sec,
        'hash': text_hash,
        'file_name': file_name,
        'speaker': speaker
    }


def adjust_time_stamps(chapters):
    # For each chapter make sure that time difference between start_time and start_time is equal to duration of MP3
    for chapter in chapters:
        chapter['end_time'] = chapter['start_time'] + chapter['mp3_duration_sec']

    # For each chapter make sure that it's end_time is greater or equal than
    # start_time of the previous chapter, if not adjust start and end time of the chapter
    for i in range(1, len(chapters)):
        if chapters[i]['start_time'] < chapters[i-1]['end_time']:
            chapters[i]['start_time'] = chapters[i-1]['end_time']
            chapters[i]['end_time'] = chapters[i]['start_time'] + chapters[i]['mp3_duration_sec']


def make_voiceover(param):
    output_dir = os.path.expanduser(param.output_dir)
    srt_name = os.path.expanduser(param.srt_name)
    require_voice_name = param.require_voice_name
    default_voice_id = VOICE_IDS['Paul']

    with open('elevenlabs-key.txt', 'rt') as key_file:
        key = key_file.read().strip()

    chapters = []
    with open(srt_name, 'rt', encoding="utf-8-sig") as srt_file:
        got_number = False
        got_time = False
        text = ''
        for line in srt_file:
            line = line.strip()
            if not got_number:
                if require_voice_name:
                    voice_id = None
                number = int(line)
                got_number = True
            elif not got_time:
                start_time, end_time = decode_time(line)
                got_time = True
            elif line == '':
                print(f'Chapter {number}:\n[{text}]')
                # Find the name of the speaker denoted as [name]: in the text
                text, voice_id, speaker = get_voice_id(require_voice_name, text, default_voice_id)
                file_name = make_voiceover_file(number, text, key, voice_id, output_dir)
                chapters.append(make_chapter(number, start_time, end_time, text, file_name, speaker))
                got_number = False
                got_time = False
                text = ''
            else:
                text += line + '\n'

        # Process the last chapter
        if text != '':
            print(f'Chapter {number}:\n[{text}]')
            text, voice_id, speaker = get_voice_id(require_voice_name, text, default_voice_id)
            file_name = make_voiceover_file(number, text, key, voice_id, output_dir)
            chapters.append(make_chapter(number, start_time, end_time, text, file_name, speaker))

    adjust_time_stamps(chapters)

    # Make new .SRT file with adjusted time stamps
    # Create new .SRT file name
    srt_output_name = os.path.splitext(srt_name)[0] + '_adjusted.srt'
    with open(srt_output_name, 'wt', encoding="utf-8") as srt_output_file:
        for chapter in chapters:
            srt_output_file.write(f"{chapter['number']}\n")
            srt_output_file.write(
                f"{chapter['start_time'].strftime('%H:%M:%S,000')} --> {chapter['end_time'].strftime('%H:%M:%S,000')}\n")
            if chapter['speaker'] is not None:
                srt_output_file.write(f"[{chapter['speaker']}]: ")
            srt_output_file.write(f"{chapter['text']}\n\n")

    # Make sure that chapter number in file name matches actual chapter number
    for chapter in chapters:
        file_name = chapter['file_name']
        t = os.path.splitext(os.path.basename(file_name))[0].split('-')
        file_no = int(t[0])
        chapter_no = int(chapter['number'])
        if file_no != chapter['number']:
            desired_name = output_dir + os.sep + f'{chapter_no:03d}-' + t[1] + '-' + t[2] + '.mp3'
            print(f'Renaming {file_name} to {desired_name}')
            os.rename(file_name, desired_name)
            chapter['file_name'] = desired_name

    proper_files = set()
    for chapter in chapters:
        proper_files.add(chapter['file_name'])

    # Move orphan MP3 files to the separate directory
    files = os.listdir(output_dir)
    for file in files:
        if file.lower().endswith('.mp3'):
            t = file.split('-')
            if len(t) > 1:
                file_name = output_dir + os.sep + file
                if file_name not in proper_files:
                    orphans_dir = output_dir + os.sep + 'orphans'
                    print(f'Moving orphan file {file_name} to {orphans_dir}')
                    os.makedirs(orphans_dir, exist_ok=True)
                    shutil.move(output_dir + os.sep + file, orphans_dir)

    print(f'{srt_output_name} created')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(fromfile_prefix_chars='@')  # Allow arguments to be read from file
    parser.add_argument("--srt-name", help="Name of .SRT file", required=True)
    parser.add_argument("--output-dir", help="Output directory", required=True)
    parser.add_argument("--require-voice-name", help="Require voice name in the text", action='store_true')
    make_voiceover(parser.parse_args())
