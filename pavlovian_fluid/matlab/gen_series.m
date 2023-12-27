% create trial sequences for pavlovian test
% 2011-06-19, (c) Paul Groot, AMC Radiologie

% [0 - Stim A/B/C/D 0-0-0-0 reward/aversive (baseline)]
% 1 - Stim A/B 80-20 reward (svp de timing vd reward bij de 20% CS pas na trial 11 doen).
% 2 - Stim C/D 20-80 aversive
% 3 - Stim A/B 20-80 reward
% 4 - Stim C/D 80-20 aversive
% 5 - Stim A/B 80-20 reward
% 6 - Stim C/D 20-80 aversive
% 7 - Stim A/B 20-80 reward
% 8 - Stim C/D 80-20 aversive

% design: [ picturenr, #trials, #rewards, pump, picturenr, #ntrials, #rewards, pump; ...]
DAR = [ ...
        1, 15, 12, 1, 2, 15,  3, 1; ...
        3, 15,  3, 2, 4, 15, 12, 2; ...
        1, 15,  3, 1, 2, 15, 12, 1; ...
        3, 15, 12, 2, 4, 15,  3, 2; ...
        1, 15, 12, 1, 2, 15,  3, 1; ...
        3, 15,  3, 2, 4, 15, 12, 2; ...
        1, 15,  3, 1, 2, 15, 12, 1; ...
        3, 15, 12, 2, 4, 15,  3, 2; ...
    ];

DRA = [ ...
        1, 15, 12, 2, 2, 15,  3, 2; ...
        3, 15,  3, 1, 4, 15, 12, 1; ...
        1, 15,  3, 2, 2, 15, 12, 2; ...
        3, 15, 12, 1, 4, 15,  3, 1; ...
        1, 15, 12, 2, 2, 15,  3, 2; ...
        3, 15,  3, 1, 4, 15, 12, 1; ...
        1, 15,  3, 2, 2, 15, 12, 2; ...
        3, 15, 12, 1, 4, 15,  3, 1; ...
    ];

D=DRA;
nSessions = 4; % how many trial sequences do you want?
nBlocks = size(D,1);
M = zeros( (sum(D(2,:))+sum(D(5,:))), nSessions * 2);
for iSession=1:nSessions
    iM = 1;
    for iBlock=1:nBlocks
        p1 = D(iBlock, 1);
        n1 = D(iBlock, 2);
        r1 = D(iBlock, 3);
        t1 = D(iBlock, 4);
        p2 = D(iBlock, 5);
        n2 = D(iBlock, 6);
        r2 = D(iBlock, 7);
        t2 = D(iBlock, 8);
        P = [ repmat(p1,1,n1) repmat(p2,1,n2)];
        R = [ repmat(t1,1,r1) zeros(1,n1-r1) repmat(t2,1,r2) zeros(1,n2-r2) ];
        % and shuffle
        [dummy I] = sort(rand(size(P)));
        M(iM:iM+n1+n2-1,(iSession-1)*2 + 1) = R(I)'; % reward vector
        M(iM:iM+n1+n2-1,(iSession-1)*2 + 2) = P(I)'; % picture vector
        iM=iM++n1+n2; % advance to next block
    end
end

xlswrite('trial-series.xls', M);
