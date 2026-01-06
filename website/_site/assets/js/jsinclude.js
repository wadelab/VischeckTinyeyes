var img = new Image();
var canvas = document.getElementById("canvas");
var ctx = canvas.getContext("2d");
var fileName = "";
var hexColor = "#ffffff";

$(document).ready(function() {
  $("#maxrgb-btn").on("click", function(e) {
    hexColor = $("#hex-color").val();
    Caman("#canvas", function() {
      this.revert(false);
      this.newLayer(function() {
        this.fillColor(hexColor);
        this.setBlendingMode("maxrgb");
      });
      this.render();
    });
  });

  $("#minrgb-btn").on("click", function(e) {
    hexColor = $("#hex-color").val();
    Caman("#canvas", function() {
      this.revert(false);
      this.newLayer(function() {
        this.fillColor(hexColor);
        this.setBlendingMode("minrgb");
      });
      this.render();
    });
  });

  $("#original-btn").on("click", function(e) {
    Caman("#canvas", function() {
      this.revert(false);
     // this.threshold(100);
      this.render();
    });
  });

  $("#protanope-btn").on("click", function(e) {
    Caman("#canvas", function() {
      this.revert(false);
      this.prot();
      this.render();
    });
  });
  
    $("#deuteranope-btn").on("click", function(e) {
    Caman("#canvas", function() {
      this.revert(false);
      this.deut();
      this.render();
    });
  });


  $("#tritanope-btn").on("click", function(e) {
    Caman("#canvas", function() {
      this.revert(false);
      this.trit();
      this.render();
    });
  });

  $("#emboss-btn").on("click", function(e) {
    Caman("#canvas", function() {
      this.revert(false);
      this.emboss();
      this.render();
    });
  });

  $("#download-btn").on("click", function(e) {
    var fileExtension = fileName.slice(-4);
    if (fileExtension == ".jpg" || fileExtension == ".png") {
      var actualName = fileName.substring(0, fileName.length - 4);
    }
    download(canvas, actualName + "-edited.jpg");
  });

  $("#upload-file").on("change", function() {
    var file = document.querySelector("#upload-file").files[0];
    var reader = new FileReader();

    if (file) {
      fileName = file.name;
      reader.readAsDataURL(file);
    }

    reader.addEventListener(
      "load",
      function() {
        img = new Image();
        img.src = reader.result;
        img.onload = function() {
          canvas.width = img.width;
          canvas.height = img.height;
          ctx.drawImage(img, 0, 0, img.width, img.height);
          $("#canvas").removeAttr("data-caman-id");
        };
      },
      false
    );
  });
});

Caman.Event.listen("processStart", function(job) {
  $(".process-message").text("Applying: " + job.name);
});

Caman.Pixel.prototype.coordinatesToLocation = Caman.Pixel.coordinatesToLocation;
Caman.Pixel.prototype.locationToCoordinates = Caman.Pixel.locationToCoordinates;
Caman.Pixel.prototype.putPixelRelative = function(horiz, vert, rgba) {
  var newLoc;
  if (this.c == null) {
    throw "Requires a CamanJS context";
  }
  newLoc = this.loc + this.c.dimensions.width * 4 * (vert * -1) + 4 * horiz;
  if (newLoc > this.c.pixelData.length || newLoc < 0) {
    return;
  }
  this.c.pixelData[newLoc] = rgba.r;
  this.c.pixelData[newLoc + 1] = rgba.g;
  this.c.pixelData[newLoc + 2] = rgba.b;
  this.c.pixelData[newLoc + 3] = rgba.a;
  return true;
};
Caman.Pixel.prototype.putPixel = function(x, y, rgba) {
  var loc;
  if (this.c == null) {
    throw "Requires a CamanJS context";
  }
  loc = this.coordinatesToLocation(x, y, img.width);
  this.c.pixelData[loc] = rgba.r;
  this.c.pixelData[loc + 1] = rgba.g;
  this.c.pixelData[loc + 2] = rgba.b;
  this.c.pixelData[loc + 3] = rgba.a;
  return true;
};

Caman.Blender.register("maxrgb", function(rgbaLayer, rgbaParent) {
  return {
    r: rgbaParent.r > 128 ? 255 : rgbaParent.r - rgbaLayer.r,
    g: rgbaParent.g > 128 ? 255 : rgbaParent.g - rgbaLayer.g,
    b: rgbaParent.b > 128 ? 255 : rgbaParent.b - rgbaLayer.b
  };
});

Caman.Blender.register("minrgb", function(rgbaLayer, rgbaParent) {
  return {
    r: rgbaParent.r < 128 ? rgbaParent.r + rgbaLayer.r : 0,
    g: rgbaParent.g < 128 ? rgbaParent.g + rgbaLayer.r : 0,
    b: rgbaParent.b < 128 ? rgbaParent.r + rgbaLayer.r : 0
  };
});

Caman.Filter.register("emboss", function() {
  this.processKernel("Emboss", [-2, -1, 0, -1, 1, 1, 0, 1, 2]);
});

Caman.Filter.register("threshold", function(limit) {
  this.process("threshold", function(rgba) {
    var lumin = 0.2126 * rgba.r + 0.7152 * rgba.g + 0.0722 * rgba.b;
    rgba.r = lumin > limit ? 255 : 0;
    rgba.g = lumin > limit ? 255 : 0;
    rgba.b = lumin > limit ? 255 : 0;
  });
  return this;
});

Caman.Filter.register("greyscale", function() {
  this.process("greyscale", function(rgba) {
    var lumin = 0.2126 * rgba.r + 0.7152 * rgba.g + 0.0722 * rgba.b;
    rgba.r = lumin;
    rgba.g = lumin;
    rgba.b = lumin;
  });
  return this;
});

Caman.Filter.register("prot", function() {
  this.process("prot", function(rgba) {
    var dMat=new Array(0.0618, 0.6547, 0.2835, 0.0866, 0.9466, -0.0332, -0.0054, -0.1375, 1.1429);
    rgba.r = dMat[0] * rgba.r + dMat[1]  * rgba.g +dMat[2]*rgba.b;
     rgba.g = dMat[3] * rgba.r + dMat[4]  * rgba.g +dMat[5]*rgba.b;
     rgba.b = dMat[6] * rgba.r + dMat[7]  * rgba.g +dMat[8]*rgba.b;
  });
  return this;
});

Caman.Filter.register("deut", function() {
  this.process("deut", function(rgba) {
    var dMat=new Array(0.1756, 0.6025, 0.2220, 0.2418, 0.8240, -0.0657, 0.0165,0.0704, 0.9131);
    rgba.r = dMat[0] * rgba.r + dMat[1]  * rgba.g +dMat[2]*rgba.b;
     rgba.g = dMat[3] * rgba.r + dMat[4]  * rgba.g +dMat[5]*rgba.b;
     rgba.b = dMat[6] * rgba.r + dMat[7]  * rgba.g +dMat[8]*rgba.b;
  });
  return this;
});

Caman.Filter.register("trit", function() {
  this.process("trit", function(rgba) {
   var dMat=new Array(2.9660, -0.6798, -1.2861, -0.4364, 1.0418, 0.3946, 0.1916, 0.6421, 0.1662);

    rgba.r = dMat[0] * rgba.r + dMat[1]  * rgba.g +dMat[2]*rgba.b;
     rgba.g = dMat[3] * rgba.r + dMat[4]  * rgba.g +dMat[5]*rgba.b;
     rgba.b = dMat[6] * rgba.r + dMat[7]  * rgba.g +dMat[8]*rgba.b;
  });
  return this;
});

  //var dMat=new Array(2.9660, -0.6798, -1.2861, -0.4364, 1.0418, 0.3946, 0.1916, 0.6421, 0.1662);


 

  



Caman.Filter.register("erased", function() {
  this.process("erased", function(rgba) {
    if (Math.random() < 0.25) {
      rgba.putPixelRelative(2, 2, {
        r: 255,
        g: 255,
        b: 255,
        a: 255
      });
    }
  });
  return this;
});

function download(canvas, filename) {
  var e;
  var lnk = document.createElement("a");

  lnk.download = filename;

  lnk.href = canvas.toDataURL("image/jpeg", 0.8);

  if (document.createEvent) {
    e = document.createEvent("MouseEvents");
    e.initMouseEvent(
      "click",
      true,
      true,
      window,
      0,
      0,
      0,
      0,
      0,
      false,
      false,
      false,
      false,
      0,
      null
    );
    lnk.dispatchEvent(e);
  } else if (lnk.fireEvent) {
    lnk.fireEvent("onclick");
  }
}

