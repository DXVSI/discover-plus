#!/bin/bash

url="https://github.com/google/material-design-icons/raw/refs/heads/master/variablefont/MaterialSymbolsOutlined%5BFILL,GRAD,opsz,wght%5D.codepoints"
dst=include/qml_material/token/icon_code.inl
curl -L "$url" > /tmp/icon_code.inl

sed -e 's|^|CODE\(|' -e 's|$|"\);|' -e 's| |, "\\u|' /tmp/icon_code.inl > $dst