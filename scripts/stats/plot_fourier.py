import pandas as pd
import matplotlib.pyplot as plt
import sys
import os

def plot_fourier_csv(csv_path):
    if not os.path.exists(csv_path):
        print(f"❌ Erreur : Le fichier '{csv_path}' est introuvable.")
        return

    try:
        df = pd.read_csv(csv_path)
    except Exception as e:
        print(f"❌ Erreur lors de la lecture du CSV : {e}")
        return

    required_cols = ['freq_idx', 'magnitude', 'real', 'imag']
    if not all(col in df.columns for col in required_cols):
        print(f"❌ Erreur : Le CSV doit contenir les colonnes {required_cols}")
        return

    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(10, 8))
    fig.suptitle(f"Analyse de Fourier - {os.path.basename(csv_path)}", fontsize=14)

    ax1.plot(df['freq_idx'], df['magnitude'], color='blue', label='Magnitude')
    ax1.set_ylabel('Magnitude')
    ax1.set_xlabel('Index de Fréquence (k)')
    ax1.grid(True, linestyle='--', alpha=0.7)
    ax1.legend()

    ax2.plot(df['freq_idx'], df['real'], color='red', alpha=0.7, label='Réel')
    ax2.plot(df['freq_idx'], df['imag'], color='green', alpha=0.7, label='Imaginaire')
    ax2.set_ylabel('Amplitude')
    ax2.set_xlabel('Index de Fréquence (k)')
    ax2.grid(True, linestyle='--', alpha=0.7)
    ax2.legend()

    plt.tight_layout(rect=[0, 0.03, 1, 0.95])

    output_png = csv_path.replace('.csv', '.png')
    plt.savefig(output_png, dpi=300)
    print(f"Graphique sauvegardé sous : {output_png}")
    
    # plt.show()

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python3 plot_fourier_csv.py <chemin_du_csv>")
    else:
        plot_fourier_csv(sys.argv[1])