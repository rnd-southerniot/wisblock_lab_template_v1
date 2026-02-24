#!/bin/bash

REPO_NAME=$(basename $(git rev-parse --show-toplevel 2>/dev/null || pwd))
SESSION="${REPO_NAME}-claude"

if tmux has-session -t $SESSION 2>/dev/null; then
    tmux attach -t $SESSION
    exit
fi

tmux new-session -d -s $SESSION
tmux rename-window -t $SESSION:0 "team"

INDEX=0

for file in agents/*.md; do
    if [ $INDEX -eq 0 ]; then
        tmux send-keys -t $SESSION:0.0 \
        "claude --system-prompt \"\$(cat $file)\"" C-m
    else
        tmux split-window -t $SESSION
        tmux select-layout tiled
        tmux send-keys -t $SESSION:0.$INDEX \
        "claude --system-prompt \"\$(cat $file)\"" C-m
    fi
    INDEX=$((INDEX+1))
done

tmux attach -t $SESSION
