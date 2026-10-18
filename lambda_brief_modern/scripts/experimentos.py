#!/usr/bin/env python3
"""
experimentos.py — onde ficam os resultados do lambda_brief_modern.

Organização (desde 17/10/2026):

  experimentos/
    INDICE.md                    uma linha por experimento, o mais novo no topo
    AAAA-MM-DD_<nome>/           uma pasta por experimento (data em que começou)
      COMANDO.txt                cada comando que escreveu nesta pasta, com data e hora
  insumos/
    odom/                        odometria fixa da VO nova (entrada dos regimes final_*)
    altimetro/                   relatórios do altímetro real (extract_altimeter.py)

Os scripts (paper_comparison, benchmark_runner, make_videos, run_piloto.sh, …) usam
este módulo: sem --out/--output_base_dir eles criam experimentos/<hoje>_<nome>/ e
acrescentam a linha no INDICE.md sozinhos. Rodar de novo no mesmo dia reaproveita a
pasta; para continuar um experimento de outro dia, passe --continuar (pega a pasta
mais recente com o mesmo nome) ou --out <pasta>.

Uso pela linha de comando:
  python3 scripts/experimentos.py listar
  python3 scripts/experimentos.py nova <nome> "<descrição>"        # cria, registra e imprime a pasta
  python3 scripts/experimentos.py registrar <pasta> "<descrição>"  # registra uma pasta que já existe
  python3 scripts/experimentos.py onde <caminho antigo>            # para onde foi uma pasta antiga
"""

import datetime
import glob
import os
import re
import shlex
import sys

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
MODERN_DIR = os.path.dirname(SCRIPT_DIR)
EXP_DIR = os.path.join(MODERN_DIR, "experimentos")
INDICE = os.path.join(EXP_DIR, "INDICE.md")
INSUMOS_DIR = os.path.join(MODERN_DIR, "insumos")

# Pastas antigas -> novas (relativas ao lambda_brief_modern). Usado para achar
# resultados pelo caminho antigo (cmd.txt, padrões de scripts, documentação).
# A ordem importa: o prefixo mais específico vem primeiro.
MOVES = [
    ("results_final/videos", "experimentos/2026-10-15_piloto_baro_vs_noalt/videos"),
    ("results_final/B_piloto", "experimentos/2026-10-15_piloto_baro_vs_noalt"),
    ("results_final/apresentacao_15_10", "experimentos/2026-10-15_apresentacao_piloto"),
    ("results_final/C_artigo_novo_a1", "experimentos/2026-10-17_artigo_novo_a1"),
    ("results_final/odom", "insumos/odom"),
    ("results_final/altimeter", "insumos/altimetro"),
    ("resultados/results_voo_completo_01", "experimentos/2026-09-12_voo_completo_01"),
    ("resultados/results_voo_completo_02", "experimentos/2026-09-14_voo_completo_02"),
    ("resultados/results_ufrgs", "experimentos/2026-09-17_ufrgs_primeiro_teste"),
    ("resultados/results_vis_adm0036", "experimentos/2026-09-27_adm2_seed42_video"),
    ("resultados/results_adm0036_alt", "experimentos/2026-09-27_adm2_prior_escala"),
    ("resultados/analise_fim", "experimentos/2026-09-27_analise_fim_E1-E5"),
    ("resultados/results_2fases_s1", "experimentos/2026-09-28_duas_fases_seed1"),
    ("resultados/results_todos_mapas", "experimentos/2026-09-28_todos_mapas_adm2"),
    ("resultados/results_benchmark", "experimentos/2026-10-01_benchmark_12-09_parcial"),
    ("results_benchmark", "experimentos/2026-10-01_benchmark_artigo_vanilla"),
    ("results_odometria_vo_mapas", "experimentos/2026-10-13_odometria_vo_mapa_17-08"),
    ("results_odometria_vo", "experimentos/2026-10-12_odometria_vo_variantes"),
]

_HEADER = """# Índice dos experimentos

Uma linha por experimento, o mais novo no topo. A data é a do início do experimento.
Os scripts acrescentam a linha sozinhos (ver `scripts/experimentos.py`). Cada pasta
tem um `COMANDO.txt` com os comandos que escreveram nela.

| Data | Pasta | O que é | Comando |
|---|---|---|---|
"""


# --------------------------------------------------------------------------- caminhos

def _rel(path):
    """Caminho relativo ao lambda_brief_modern, com '/', ou None se estiver fora dele."""
    ap = os.path.abspath(path)
    try:
        rel = os.path.relpath(ap, MODERN_DIR)
    except ValueError:
        return None
    if rel.startswith(".."):
        return None
    return rel.replace(os.sep, "/")


def canonical(path):
    """Caminho no layout NOVO, exista ou não (para comparar comandos antigos e novos)."""
    if not isinstance(path, str) or not path:
        return path
    rel = _rel(path) if os.path.isabs(path) or os.sep in path else None
    if rel is None:
        return path
    for old, new in MOVES:
        if rel == old or rel.startswith(old + "/"):
            return os.path.join(MODERN_DIR, new + rel[len(old):])
    return path


def resolve(path):
    """Devolve o caminho se ele existe; senão, onde ele está no layout novo (se lá existir)."""
    if not isinstance(path, str) or not path or os.path.exists(path):
        return path
    new = canonical(path)
    return new if new != path and os.path.exists(new) else path


def _insumo(new_rel, old_rel):
    new, old = os.path.join(MODERN_DIR, new_rel), os.path.join(MODERN_DIR, old_rel)
    # Antes de mover as pastas antigas, continua usando a antiga (não cria uma segunda cópia).
    return old if (os.path.isdir(old) and not os.path.isdir(new)) else new


def odom_dir():
    return _insumo("insumos/odom", "results_final/odom")


def altimetro_dir():
    return _insumo("insumos/altimetro", "results_final/altimeter")


# --------------------------------------------------------------------------- experimentos

def slug(nome):
    s = re.sub(r"[^\w.\-]+", "_", nome.strip(), flags=re.UNICODE).strip("_")
    return s or "experimento"


def _ultima(nome):
    pastas = sorted(p for p in glob.glob(os.path.join(EXP_DIR, f"????-??-??_{slug(nome)}")) if os.path.isdir(p))
    return pastas[-1] if pastas else None


def pasta_experimento(nome, descricao, comando=None, continuar=False, data=None, dry_run=False):
    """Cria (ou reaproveita) experimentos/<data>_<nome>/, registra no INDICE.md e anota o comando.

    continuar=True reaproveita a pasta mais recente com o mesmo nome, de qualquer data.
    dry_run=True só devolve o caminho, sem criar nem registrar nada.
    """
    pasta = _ultima(nome) if continuar else None
    if continuar and pasta is None:
        print(f"[experimentos] nenhuma pasta anterior '*_{slug(nome)}'; criando uma nova.", file=sys.stderr, flush=True)
    if pasta is None:
        data = data or datetime.date.today().isoformat()
        pasta = os.path.join(EXP_DIR, f"{data}_{slug(nome)}")
    if dry_run:
        return pasta
    os.makedirs(pasta, exist_ok=True)
    registrar(pasta, descricao, comando)
    return pasta


def _cmd_str(comando):
    if comando is None:
        comando = sys.argv
    if isinstance(comando, (list, tuple)):
        parts = list(comando)
        if parts and parts[0].endswith(".py"):
            parts[0] = os.path.relpath(parts[0], MODERN_DIR) if os.path.isabs(parts[0]) else parts[0]
            parts = ["python3"] + parts
        return " ".join(shlex.quote(str(p)) for p in parts)
    return str(comando)


def registrar(pasta, descricao, comando=None):
    """Acrescenta a pasta no INDICE.md (se ainda não estiver) e o comando no COMANDO.txt dela."""
    pasta = os.path.abspath(pasta)
    os.makedirs(pasta, exist_ok=True)
    cmd = _cmd_str(comando)
    agora = datetime.datetime.now().strftime("%Y-%m-%d %H:%M")
    with open(os.path.join(pasta, "COMANDO.txt"), "a") as f:
        f.write(f"# {agora}  (cwd: {os.getcwd()})\n{cmd}\n\n")

    os.makedirs(EXP_DIR, exist_ok=True)
    texto = open(INDICE).read() if os.path.exists(INDICE) else _HEADER
    nome = os.path.basename(pasta)
    if f"]({nome}/)" in texto:
        return False
    m = re.match(r"(\d{4}-\d{2}-\d{2})_", nome)
    data = m.group(1) if m else agora[:10]
    if _rel(pasta) and not _rel(pasta).startswith("experimentos/"):
        link = os.path.relpath(pasta, EXP_DIR).replace(os.sep, "/")
    else:
        link = nome
    curto = cmd if len(cmd) <= 140 else cmd[:137] + "..."
    linha = (f"| {data} | [{nome}]({link}/) | {descricao.replace('|', '/')} | "
             f"`{curto.replace('|', '/').replace('`', '')}` |\n")
    linhas = texto.splitlines(keepends=True)
    sep = next((i for i, l in enumerate(linhas) if l.startswith("|---")), None)
    if sep is None:
        texto = texto.rstrip("\n") + "\n\n" + _HEADER.split("\n\n", 2)[-1] + linha
    else:
        linhas.insert(sep + 1, linha)
        texto = "".join(linhas)
    with open(INDICE, "w") as f:
        f.write(texto)
    print(f"[experimentos] registrado em experimentos/INDICE.md: {nome}", file=sys.stderr, flush=True)
    return True


# --------------------------------------------------------------------------- CLI

def _main(argv):
    if not argv or argv[0] in ("-h", "--help"):
        print(__doc__)
        return 0
    acao, resto = argv[0], argv[1:]
    if acao == "listar":
        if not os.path.exists(INDICE):
            print("Ainda não há experimentos/INDICE.md.")
            return 1
        for l in open(INDICE):
            if l.startswith("| 20"):
                c = [x.strip() for x in l.strip().strip("|").split("|")]
                nome = re.sub(r"^\[([^]]+)\].*", r"\1", c[1])
                print(f"{c[0]}  {nome:45s}  {c[2]}")
        return 0
    if acao == "nova" and len(resto) >= 2:
        print(pasta_experimento(resto[0], resto[1], comando=resto[2:] or ["(manual)"]))
        return 0
    if acao == "registrar" and len(resto) >= 2:
        registrar(resto[0], resto[1], comando=resto[2:] or ["(manual)"])
        return 0
    if acao == "onde" and resto:
        for p in resto:
            novo = canonical(os.path.abspath(p))
            existe = "existe" if os.path.exists(novo) else "ainda não existe"
            print(f"{p} -> {os.path.relpath(novo, os.getcwd())} ({existe})")
        return 0
    print(__doc__)
    return 2


if __name__ == "__main__":
    sys.exit(_main(sys.argv[1:]))
