clc;
clear;
close all;
% addpath(fullfile(pwd, 'libsdca-debug'));
addpath(fullfile(pwd, 'libsdca-release'));
% addpath('/BS/mlapin-projects3/work/cvpr16/code/src/utility');
rng(0);

if 1
W = randn(4096,200);
X = randn(4096,50000);
Y = randi(200,50000,1);
opts.objective = 'l2_topk_softmax_nonconvex';
opts.max_epoch = 5;
opts.log_level = 'debug';
opts.log_format = 'long_e';
model = libsdca_gd(X, Y, opts);
end

if 0
load('/BS/mlapin-projects3/work/cvpr16/code/experiments/caltech101sil/original/data/trn.mat');
tst = load('/BS/mlapin-projects3/work/cvpr16/code/experiments/caltech101sil/original/data/tst.mat');

opts.k = 1;
opts.C = 0.1;
opts.log_level = 'debug';
opts.log_format = 'long_e';
opts.max_epoch = 1000;
opts.epsilon = 1e-10;

model = libsdca_solve_mt({X,tst.X},{Y,tst.Y},opts);
end

if 0
load('/BS/mlapin-projects3/work/cvpr16/code/experiments/caltech101sil/original/l2_entropy_topk/models/model-l2_entropy_topk-k-3-C-0.1.mat');

m = model;

d = size(X,1);
n = size(X,2);
T = numel(unique(Y));

opts.k = m.k;
opts.C = m.C; %1;
opts.W = m.W; %zeros(d,T);
opts.log_level = 'verbose';
opts.log_format = 'long_e';
opts.max_epoch = 50;

model = libsdca_gd_mt(X,Y,opts);

atrn0 = allaccuracies(opts.W'*X, Y);
atrn1 = allaccuracies(model.W'*X, Y);
atst0 = allaccuracies(opts.W'*tst.X, tst.Y);
atst1 = allaccuracies(model.W'*tst.X, tst.Y);

fprintf('trn:\n');
fprintf('before: %.1f %.1f %.1f %.1f %.1f\n', 100*atrn0(1:5));
fprintf('after : %.1f %.1f %.1f %.1f %.1f\n', 100*atrn1(1:5));
fprintf('tst:\n');
fprintf('before: %.1f %.1f %.1f %.1f %.1f\n', 100*atst0(1:5));
fprintf('after : %.1f %.1f %.1f %.1f %.1f\n', 100*atst1(1:5));
end

return;

%%%
%%% Test runtraining
%%%
% p = pwd;
% cd /BS/mlapin-projects3/work/cvpr16/code/src
% myinit;
% cd ..
% runtraining('flowers/gaurav','l2_topk_hinge','gamma','0','k','15','C','0.1');
% cd(p);
% 
% return;

if usejava('jvm') && ~exist('cvx_begin', 'file') ...
    && exist(fullfile('cvx', 'cvx_startup.m'), 'file')
  addpath('prox-cvx');
  run(fullfile('cvx', 'cvx_startup.m'));
  % SeDuMi is usually faster, but SDPT3 may be more accurate
  cvx_solver sedumi
%   cvx_solver sdpt3;
end

%%%
%%% Test prox on real data
%%%
if 0
%   load('data/prox_topk_simplex_biased.mat');
  load('data/prox_topk_entropy_biased.mat');
  
  A0 = A;
  opts0 = opts;
  for i = 8
    opts.alpha = opts0.alpha(i);
    A = A0(:,i);
    X1 = libsdca_prox(A, opts);
    [X2,info] = prox_cvx(A, opts);

    disp(opts);
    loss = info.loss;
    fprintf('Loss (lower is better):\n');
    fprintf('   solver = %+.16e\n', loss(A,X(:,i)));
    fprintf('      lib = %+.16e\n', loss(A,X1));
    fprintf('      cvx = %+.16e\n', loss(A,X2));
    fprintf('cvx - lib = %+.16e\n', loss(A,X2) - loss(A,X1));
    fprintf('Solution difference:\n');
    fprintf('     RMSD = %+.16e\n', norm(X1-X2,'fro')/sqrt(numel(X1)));
  end
end

%%%
%%% Test prox
%%%
if 0
  d = 10;
  n = 10;

%   opts.prox = 'entropy';
%   opts.prox = 'topk_cone_biased';
%   opts.prox = 'knapsack';
%   opts.prox = 'lambert_w_exp';
  opts.prox = 'topk_entropy';
%   opts.prox = 'topk_entropy_biased';
%   opts.prox = 'topk_simplex_biased';
  opts.k = 10;
%   opts.alpha = 1e+3;
%   opts.summation = 'kahan';
%   opts.rhs = 1;
%   opts.hi = 1;

%   A = bsxfun(@times, ones(d,n), randn(1,n));
  A = randn(d,n) + 10*randn(d,n) + 100*randn(d,n) + 1000*randn(d,n);
%   A = -10:0.01:10;
  B = libsdca_prox(A, opts);
  
  if exist('cvx_begin', 'file')
    [X,info] = prox_cvx(A, opts);

  % [X,mu,nu] = prox_cvx_entropy(A, opts);
  % loss = @(A,X) 0.5*sum(sum((A - X).^2)) - sum(sum(entr(X)));

    disp(opts);
    loss = info.loss;
    fprintf('Loss (lower is better):\n');
    fprintf('      lib = %+.16e\n', loss(A,B));
    fprintf('      cvx = %+.16e\n', loss(A,X));
    fprintf('cvx - lib = %+.16e\n', loss(A,X) - loss(A,B));
    fprintf('Solution difference:\n');
    fprintf('     RMSD = %+.16e\n', norm(B-X,'fro')/sqrt(numel(B)));
    sum(B)
    sum(X)
    k=opts.k;

  end  
%   T = zeros(100,1);
%   for k=1:100
%     opts.k = k;
%     t = tic;
%     B = libsdca_prox(A, opts);
%     T(k) = toc(t);
%   end
%     plot(T)
%   plot(sum(B))
%   disp(sum(B));
  
%   [X,mu,nu] = prox_entropy_cvx(A, opts.hi, opts.rhs);
%   
%   loss = @(X) 0.5*sum(sum((A - X).^2)) - sum(sum(entr(X)));
%   disp(loss(X)-loss(B));
end

%%%
%%% Run test cases
%%%
if 0
  cd /BS/mlapin-projects1/work/simplex/test
  runtestcases_2
end

%%%
%%% Test solver
%%%
if 1
%   load('data/sun397-cnn.mat');
%   load('data/sun397-cnn-trn.mat');
%   load('data/sun397-cnn-tst.mat');
%   load('data/sun397-fv.mat'); % converges
  load('data/sun397-fv-trn.mat');
  load('data/sun397-fv-tst.mat');
%   load('data/indoor67-cnn-trn.mat'); % no convergence
%   load('data/indoor67-cnn-tst.mat');

%   Xtrn = double(Xtrn);
% 	Xtst = double(Xtst);
% 
%   Xc = mean(Xtrn,2);
%   Xtrn = bsxfun(@minus, Xtrn, Xc);
%   Xtst = bsxfun(@minus, Xtst, Xc);

%   Xtrn = [Xtrn; ones(1, size(Xtrn,2))];
%   Xtst = [Xtst; ones(1, size(Xtst,2))];

 Ktrn = double(Ktrn);
 Ktst = double(Ktst);
%   Ktrn = Ktrn-1;
  
%   ix = 1:5*2;
%   Ktrn = Ktrn(ix,ix);
%   Ytrn = Ytrn(ix);

  opts.objective = 'l2_entropy_topk';
%   opts.objective = 'l2_topk_hinge';
%   opts.objective = 'l2_hinge_topk';
  opts.C = 1e-1;
  opts.k = 2;
  opts.gamma = 1;
  opts.epsilon = 1e-15;
  opts.check_on_start = 0;
  opts.check_epoch = 1;
  opts.max_epoch = 20;
  opts.summation = 'standard';
  opts.precision = 'double';
  opts.log_level = 'debug';
  opts.log_format = 'long_e';
  opts.is_dual = 1;

  if opts.is_dual
    if ~exist('Ktrn', 'var') && exist('Xtrn', 'var')
      Ktrn = Xtrn'*Xtrn;
    end
    if ~exist('Ktst', 'var') && exist('Xtst', 'var')
      Ktst = Xtrn'*Xtst;
    end
    if exist('Ktst', 'var')
      model = libsdca_solve({Ktrn, Ktst}, {Ytrn, Ytst}, opts);
    else
      model = libsdca_solve(Ktrn, Ytrn, opts);
    end
    disp(model);
    [~,pred] = max(model.A*Ktrn);
    fprintf('accuracy: %g\n', 100*mean(pred(:) == Ytrn(:)));
  else
    if exist('Xtst', 'var')
      model = libsdca_solve({Xtrn, Xtst}, {Ytrn, Ytst}, opts);
    else
      model = libsdca_solve(Xtrn, Ytrn, opts);
    end
    disp(model);
    [~,pred] = max(model.W'*Xtrn);
    fprintf('accuracy: %g\n', 100*mean(pred(:) == Ytrn(:)));
  end
  
end
