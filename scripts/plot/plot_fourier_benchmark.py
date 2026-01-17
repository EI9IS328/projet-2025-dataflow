import pandas as pd
import matplotlib.pyplot as plt

def plot_benchmark(csv_file):
    try:
        df = pd.read_csv(csv_file)
        
        print("Données chargées :")
        print(df)

        plt.figure(figsize=(10, 6))

        plt.plot(df['Sizes'], df['Insitu (ms)'], 
                 marker='o', linestyle='-', linewidth=2, label='In-situ')

        plt.plot(df['Sizes'], df['Adhoc (ms)'], 
                 marker='s', linestyle='--', linewidth=2, label='Ad-hoc')

        plt.title('Performance Benchmark: In-situ vs Ad-hoc', fontsize=14)
        plt.xlabel('Taille du problème (Size)', fontsize=12)
        plt.ylabel('Temps d\'exécution (ms)', fontsize=12)
        
        plt.grid(True, which='both', linestyle='--', alpha=0.7)
        
        plt.legend(fontsize=12)

        output_image = './data/fourier/plot_benchmark.png'
        plt.savefig(output_image)
        print(f"\nGraphique sauvegardé sous : {output_image}")
        
        plt.show()

    except FileNotFoundError:
        print(f"Erreur : Le fichier '{csv_file}' est introuvable.")
    except Exception as e:
        print(f"Une erreur est survenue : {e}")

if __name__ == "__main__":
    plot_benchmark('data/fourier/resultats_benchmark_fourier.csv')