import argparse
import hashlib
import re
import os
import requests
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

    # Check if the file with the same hash already exists in the output directory
    files = os.listdir(output_dir)
    for file in files:
        t = file.split('-')
        if len(t) > 1 and t[2] == text_hash + '.mp3':
            print(f'File {file_name} already exists, skipping ...')
            return

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
        return

    with open(file_name, 'wb') as f:
        for chunk in response.iter_content(chunk_size=CHUNK_SIZE):
            if chunk:
                f.write(chunk)

    print(f'File name: {file_name} created')


def make_voiceover(param):
    output_dir = os.path.expanduser(param.output_dir)
    srt_name = os.path.expanduser(param.srt_name)
    require_voice_name = param.require_voice_name
    default_voice_id = VOICE_IDS['Pirate']

    with open('elevenlabs-key.txt', 'rt') as key_file:
        key = key_file.read().strip()

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
                got_time = True
            elif line == '':
                print(f'Chapter {number}:\n[{text}]')
                # Find the name of the speaker denoted as [name]: in the text
                text, voice_id = get_voice_id(require_voice_name, text, default_voice_id)
                make_voiceover_file(number, text, key, voice_id, output_dir)
                got_number = False
                got_time = False
                text = ''
            else:
                text += line + '\n'

        # Process the last chapter
        if text != '':
            print(f'Chapter {number}:\n[{text}]')
            text, voice_id = get_voice_id(require_voice_name, text, default_voice_id)
            make_voiceover_file(number, text, key, voice_id, output_dir)


def get_voice_id(require_voice_name, text, default_voice_id):
    speaker = re.search(r'\[(.*?)\]:', text)
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

    return text, default_voice_id


if __name__ == '__main__':
    parser = argparse.ArgumentParser(fromfile_prefix_chars='@')  # Allow arguments to be read from file
    parser.add_argument("--srt-name", help="Name of .SRT file", required=True)
    parser.add_argument("--output-dir", help="Output directory", required=True)
    parser.add_argument("--require-voice-name", help="Require voice name in the text", action='store_true')
    make_voiceover(parser.parse_args())
