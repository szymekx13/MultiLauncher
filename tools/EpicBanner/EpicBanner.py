from steamgrid import SteamGridDB
from pathlib import Path
import requests
import os
from dotenv import load_dotenv

def download_image(game_name):
    # Inicjalizacja API
    load_dotenv()
    api_key = os.getenv('STEAMGRIDDB_API_KEY')
    if not api_key:
        print("Error: STEAMGRIDDB_API_KEY not found in .env file")
        return False
    sgdb = SteamGridDB(api_key)

    # Szukanie gry
    result = sgdb.search_game(game_name)

    if not result:
        return False

    game_zero = result[0] # we assume that the first result is the correct game
    try:
        grids = sgdb.sgdb.get_grids_by_gameid(
                game_ids=[game_zero.id]
        )
        banner_url = None
        if grids:
            for img in grids:
                if hasattr(img, 'width') and hasattr(img, 'height'):
                    if img.width > img.height:
                        banner_url = img.url
                        break
            if not banner_url:
                banner_url = grids[0].url
                
            # Używamy ścieżki relatywnej do Twojego projektu
            path = Path("assets/cache")
            filename = f"{game_zero.id}_hero.jpg"
            save_path = path / filename

            save_path.parent.mkdir(parents=True, exist_ok=True)

            try:
                response = requests.get(banner_url, timeout=10)
                response.raise_for_status()
                with open(save_path, 'wb') as f:
                    f.write(response.content)
                print(f"Image downloaded and saved to {save_path}")
                return str(save_path)
            except Exception as e:
                print(f"Error downloading image: {e}")
    except Exception as e:
        print(f"API error: {e}")
        return False
    return False