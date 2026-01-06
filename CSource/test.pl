#!/usr/bin/perl
# ./run.pl img.jpg 32 32 

#use Image::Info qw(image_info dim);

$inFile = "testImage.jpg";
$w = 640 ;
$h = 512;
$sensor = 'deut';
$viewDist = 200;

#$outFile = "/home/bob/public_html/test.jpg";
$outFile = "test_".$sensor.".jpg";

#my $info = image_info("$inFile");
#%info = %$info;
#$w = $info{"width"};
#$h = $info{"height"};
$sz = "-size ".$w.",".$h;
#print($sz."\n");

qx(convert $inFile RGB:- | ./runVischeck3 -v -m $w,$h -t $sensor -d $viewDist -r 90 | rawtoppm -rgb $w $h - | ppmtojpeg --quality=80 --optimize --progressive --comment='Processed by vischeck.com' > $outFile);

