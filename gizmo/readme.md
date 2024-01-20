# Gizmo

Gizmo is a small internal tool for building things inside this project via json config
(at least for now, maybe we will migrate it to toml later)

Right now gizmo is extremely stupid. No caching, all c++ code is squashed into one
big file and compiled into one object with g++
