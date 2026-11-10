"""Syntax-check the production Founder web page with Node.js."""
from pathlib import Path
import re, subprocess, tempfile, argparse
p=argparse.ArgumentParser();p.add_argument('--node',default='node');a=p.parse_args()
root=Path(__file__).resolve().parents[2]
html=(root/'src/founder/page.h').read_text(encoding='utf-8')
scripts=re.findall(r'<script>(.*?)</script>',html,re.S)
assert scripts, 'No embedded JavaScript found'
with tempfile.TemporaryDirectory(prefix='grova-founder-web-') as temp:
    for i,script in enumerate(scripts):
        path=Path(temp)/f'page-{i}.js';path.write_text(script,encoding='utf-8')
        subprocess.run([a.node,'--check',str(path)],check=True)
print('Founder web JavaScript syntax passed')
